#include <boost/lexical_cast.hpp>
#include "client_connection.h"
#include "http_request.h"
#include "log.h"
#include "resource_manager.h"
#include "session.h"

ClientConnection::ClientConnection(std::shared_ptr<Session> session)
    : Connection(session, kSocketTimeout, kBufferSize) {}

void ClientConnection::init() {
    message_.reset(new HttpRequest(shared_from_this()));
    connected_ = true;
}

void ClientConnection::OnMessageExchangeComplete() {
    if (writing_) {
        std::lock_guard<std::mutex> lock(lock_);
        if (writing_) {
            service_.post(std::bind(&ClientConnection::OnMessageExchangeComplete,
                                    std::static_pointer_cast<ClientConnection>(shared_from_this())));
            return;
        }
    }

    if (message_->KeepAlive()) {
        reset();
        StartTimer();
        read();
    } else {
        XDEBUG_WITH_ID << "The client side does not want to keep the connection alive, destroy the session...";
        DestroySession();
    }
}

void ClientConnection::OnBodyComplete() {
    XDEBUG_WITH_ID << "The request is completed.";
    std::shared_ptr<Session> session(session_.lock());
    if (session) {
        ParseRemotePeer(session);
        service_.post(std::bind(&Session::OnRequestComplete, session, message_));
    }
}

void ClientConnection::ParseRemotePeer(const std::shared_ptr<Session>& session) {
    if (session->https() || !message_->HeaderCompleted())
        return;

    auto request = std::static_pointer_cast<HttpRequest>(message_);
    auto& method = request->method();
    if (method.length() == 7 && method[0] == 'C' && method[1] == 'O') {
        host_ = request->url();
        port_ = 443;

        auto sep = host_.find(':');
        if (sep != std::string::npos) {
            port_ = boost::lexical_cast<unsigned short>(host_.substr(sep + 1));
            host_ = host_.substr(0, sep);
        }
    } else {
        port_ = 80;
        if (!message_->headers().find("Host", host_)) {
            XERROR_WITH_ID << "Host header not found, this should never happen.";
            DestroySession();
            return;
        }

        auto sep = host_.find(':');
        if (sep != std::string::npos) {
            port_ = boost::lexical_cast<unsigned short>(host_.substr(sep + 1));
            host_ = host_.substr(0, sep);
        }
    }
}

void ClientConnection::WriteSSLReply(const std::string& reply) {
    XDEBUG_WITH_ID << "Writing SSL reply to client...";
    auto buf = std::make_shared<boost::asio::streambuf>();
    std::ostream out(buf.get());
    out << reply;

    std::lock_guard<std::mutex> lock(lock_); // TODO in fact, there is no need to lock
    assert(buffer_out_.empty());
    assert(!writing_);
//    if (!buffer_out_.empty()) {
//        XERROR_WITH_ID << "Hey, the out buffers are not empty while writing SSL reply!";
//        DestroySession();
//        return;
//    }
//    if (writing_) {
//        XERROR_WITH_ID << "Oh, connection is writing while writing SSL reply!";
//        DestroySession();
//        return;
//    }

    buffer_out_.push_back(buf);

    writing_ = true;
    socket_->async_write_some(boost::asio::buffer(buffer_out_.front()->data(),
                                                  buffer_out_.front()->size()),
                              std::bind(&ClientConnection::OnSSL,
                                        std::static_pointer_cast<ClientConnection>(shared_from_this()),
                                        std::placeholders::_1,
                                        std::placeholders::_2));
}

void ClientConnection::OnRead(const boost::system::error_code& e, std::size_t) {
    if (timer_triggered_) {
        XDEBUG_WITH_ID << "Client socket timed out, abort reading.";
        DestroySession();
        return;
    }

    if (SessionInvalidated()) {
        XDEBUG_WITH_ID << "Session is invalidated, abort reading.";
        CancelTimer();
        return;
    }

    if(e) {
        if(e == boost::asio::error::eof || SSL_SHORT_READ(e))
            XDEBUG_WITH_ID << "The other peer closed the socket, stop.";
        else
            XERROR_WITH_ID << "Error occurred during reading from socket, message: "
                           << e.message();
        CancelTimer();
        DestroySession();
        return;
    }

    CancelTimer();

    if(buffer_in_.size() <= 0) {
        XWARN_WITH_ID << "No data in buffer.";
        DestroySession();
        return;
    }

    XDEBUG_WITH_ID_X << "OnRead() called in client connection, dump content:\n"
                     << std::string(boost::asio::buffer_cast<const char*>(buffer_in_.data()),
                                    buffer_in_.size());

    ConstructMessage();
}

void ClientConnection::OnWritten(const boost::system::error_code& e, std::size_t length) {
    if(e) {
        XERROR_WITH_ID << "Error occurred during writing to client socket, message: "
                       << e.message();
        DestroySession();
        return;
    }

    XDEBUG_WITH_ID << "Content has been written to client connection.";

    std::lock_guard<std::mutex> lock(lock_);
    buffer_out_.front()->consume(length);
    if (buffer_out_.front()->size() > 0) {
        XWARN_WITH_ID << "The writing operation does not write all data in out buffer!";
        write();
        return;
    }
    buffer_out_.pop_front();
    if (!buffer_out_.empty()) {
        XDEBUG_WITH_ID << "Seems other write operations added data to out buffers.";
        write();
        return;
    }

    writing_ = false;
}

void ClientConnection::OnTimeout(const boost::system::error_code& e) {
    if (e == boost::asio::error::operation_aborted)
        XDEBUG_WITH_ID << "The client timeout timer is cancelled.";
    else if (e)
        XERROR_WITH_ID << "Error occurred with client timer, message: "
                       << e.message();
    else if (timer_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
        XDEBUG_WITH_ID << "Client socket timed out, close it.";
        DestroySession();
    }
}

void ClientConnection::OnSSL(const boost::system::error_code& e, std::size_t length) {
    if (e) {
        XERROR_WITH_ID << "Error occurred during writing SSL OK reply to client connection, message: "
                       << e.message();
        DestroySession();
        return;
    }

    std::lock_guard<std::mutex> lock(lock_); // TODO lock is not needed
    buffer_out_.front()->consume(length);
    if (buffer_out_.front()->size() > 0) {
        XWARN_WITH_ID << "The SSL writing operation does not write all data in out buffer!";
        socket_->async_write_some(boost::asio::buffer(buffer_out_.front()->data(),
                                                      buffer_out_.front()->size()),
                                  std::bind(&ClientConnection::OnSSL,
                                            std::static_pointer_cast<ClientConnection>(shared_from_this()),
                                            std::placeholders::_1,
                                            std::placeholders::_2));
        return;
    }
    buffer_out_.pop_front();
    assert(buffer_out_.empty());
    writing_ = false;

    XDEBUG_WITH_ID << "SSL reply has been written to client, now start reading...";

    auto ca = ResourceManager::GetCertManager().GetCertificate(host_);
    auto dh = ResourceManager::GetCertManager().GetDHParameters();
    socket_->SwitchProtocol(kHttps, kServer, ca, dh);

    // because we have decoded "CONNECT..." request, so we reset it here
    // to decode new request
    message_->reset();

    read();
}
