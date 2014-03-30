#include "client_connection.h"
#include "http_request.h"
#include "log.h"
#include "session.h"

ClientConnection::ClientConnection(std::shared_ptr<Session> session)
    : Connection(session, kSocketTimeout, kBufferSize) {}

void ClientConnection::init() {
    message_.reset(new HttpRequest(shared_from_this()));
    connected_ = true;
}

void ClientConnection::OnMessageExchangeComplete() {
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
    if (session)
        service_.post(std::bind(&Session::OnRequestComplete, session, message_));
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

    buffer_out_.consume(length);
    if (buffer_out_.size() > 0) {
        XWARN_WITH_ID << "The writing operation does not write all data in out buffer!";
        write();
    }
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
