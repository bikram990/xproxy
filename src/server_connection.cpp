#include <boost/lexical_cast.hpp>
#include "http_response.h"
#include "log.h"
#include "resource_manager.h"
#include "server_connection.h"
#include "session.h"

ServerConnection::ServerConnection(std::shared_ptr<Session> session)
    : Connection(session, kSocketTimeout, kBufferSize), resolver_(session->service()) {}

void ServerConnection::init() {
    message_.reset(new HttpResponse(shared_from_this()));
}

void ServerConnection::OnMessageExchangeComplete() {
    if (message_->KeepAlive()) {
        StartTimer();
    } else {
        XDEBUG_WITH_ID << "The server side does not want to keep the connection alive, close it.";
        connected_ = false;
        socket_->close();
        socket_.reset(Socket::Create(service_));
    }

    reset();

}

void ServerConnection::OnBody() {
    XDEBUG_WITH_ID << "The response has new body data.";
    std::shared_ptr<Session> session(session_.lock());
    if (session)
        service_.post(std::bind(&Session::OnResponse, session, message_));
}

void ServerConnection::OnBodyComplete() {
    XDEBUG_WITH_ID << "The response is completed.";
    std::shared_ptr<Session> session(session_.lock());
    if (session)
        service_.post(std::bind(&Session::OnResponseComplete, session, message_));
}

void ServerConnection::connect() {
    std::shared_ptr<Session> s(session_.lock());
    if (s && s->https()) {
        socket_->SwitchProtocol<ResourceManager::CertManager::CAPtr,
                    ResourceManager::CertManager::DHParametersPtr>(kHttps);
    }

    try {
        boost::asio::ip::tcp::resolver::query query(host_, boost::lexical_cast<std::string>(port_));
        auto endpoint_iterator = resolver_.resolve(query);

        XDEBUG_WITH_ID << "Connecting to remote, host: " << host_
                       << ", port: " << port_ << ", resolved address: "
                       << endpoint_iterator->endpoint().address();

        socket_->async_connect(endpoint_iterator,
                               std::bind(&ServerConnection::OnConnected,
                                         std::static_pointer_cast<ServerConnection>(shared_from_this()),
                                         std::placeholders::_1));
        } catch(const boost::system::system_error& e) {
            XERROR_WITH_ID << "Failed to resolve [" << host_ << ":" << port_ << "], error: " << e.what();
            DestroySession();
        }
}

void ServerConnection::OnRead(const boost::system::error_code& e, std::size_t) {
    if (SessionInvalidated()) {
        XDEBUG_WITH_ID << "Session is invalidated, abort reading.";
        CancelTimer();
        return;
    }

    if (e) {
        if (e == boost::asio::error::eof || SSL_SHORT_READ(e)) {
            XDEBUG_WITH_ID << "Server closed the socket, this is not persistent connection.";
            connected_ = false;
            socket_->close();
        } else {
            XERROR_WITH_ID << "Error occurred during reading from server socket, message: "
                           << e.message();
            DestroySession();
            return;
        }
    }

    if(buffer_in_.size() <= 0) {
        XWARN_WITH_ID << "No data in server socket.";
        DestroySession();
        return;
    }

    XDEBUG_WITH_ID_X << "OnRead() called in server connection, dump content:\n"
                     << std::string(boost::asio::buffer_cast<const char*>(buffer_in_.data()),
                                    buffer_in_.size());

    ConstructMessage();
}

void ServerConnection::OnWritten(const boost::system::error_code& e, std::size_t length) {
    if (SessionInvalidated()) {
        XDEBUG_WITH_ID << "Session is invalidated, abort writing to server.";
        CancelTimer();
        return;
    }

    if (e) {
        XERROR_WITH_ID << "Error occurred during writing to server connection, message: "
                       << e.message();
        DestroySession();
        return;
    }

    std::lock_guard<std::mutex> lock(lock_);
    out_->consume(length);
    if (out_->size() > 0) {
        XWARN_WITH_ID << "The writing operation does not write all data in out buffer!";
        write();
        return;
    }
    out_.swap(aux_out_);
    if (out_->size() > 0) {
        XDEBUG_WITH_ID << "Seems other write operations added data to out buffers.";
        write();
        return;
    }

    XDEBUG_WITH_ID << "Data has been written to server connection, now start reading...";

    read();
}

void ServerConnection::OnTimeout(const boost::system::error_code& e) {
    if (e == boost::asio::error::operation_aborted)
        XDEBUG_WITH_ID << "The server timeout timer is cancelled.";
    else if (e)
        XERROR_WITH_ID << "Error occurred with server timer, message: "
                       << e.message();
    else if (timer_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
        XDEBUG_WITH_ID << "Server socket timed out, close it.";
        connected_ = false;
        socket_->close();
        socket_.reset(Socket::Create(service_));
    }
}

void ServerConnection::OnConnected(const boost::system::error_code& e) {
    if (e) {
        XERROR_WITH_ID << "Error occurred during connecting to remote peer, message: "
                       << e.message() << "\nDetailed info:\n"
                       << "host: " << host_ << "\n"
                       << "port: " << port_ << "\n"
                       << "request content: \n"
                       << std::string(boost::asio::buffer_cast<const char*>(out_->data()),
                                      out_->size());
        DestroySession();
        return;
    }

    XDEBUG_WITH_ID << "Remote peer connected: " << host_ << ":" << port_;
    connected_ = true;
    write();
}
