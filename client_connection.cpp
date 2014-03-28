#include "client_connection.h"
#include "http_request.h"
#include "log.h"
#include "session.h"

ClientConnection::ClientConnection(std::shared_ptr<Session> session)
    : Connection(session, 60, 2048) {} // TODO

void ClientConnection::init() {
    message_.reset(new HttpRequest(shared_from_this()));
    connected_ = true;
}
void ClientConnection::OnBodyComplete() {
    std::shared_ptr<Session> session(session_.lock());
    if (session)
        service_.post(std::bind(&Session::OnRequestComplete, session, message_));
}

void ClientConnection::OnRead(const boost::system::error_code& e) {
    XDEBUG_WITH_ID << "OnRead callback is called.";
    if (timer_triggered_) {
        XDEBUG_WITH_ID << "Client socket timed out, abort reading.";
        timer_triggered_ = false;
        return;
    }

    //if(server_.state() == ProxyServer::kStopped) {
    if (false) { // TODO change the value here
        XDEBUG_WITH_ID << "xProxy server is stopping, abort reading.";
        timer_.cancel();
        return;
    }

    if(e) {
        if(e == boost::asio::error::eof || SSL_SHORT_READ(e))
            XDEBUG_WITH_ID << "The other peer closed the socket, stop.";
        else
            XERROR_WITH_ID << "Error occurred during reading from socket, message: "
                           << e.message();
        timer_.cancel();
        // TODO add logic here
        return;
    }

    timer_.cancel();

    if(buffer_in_.size() <= 0) {
        XWARN_WITH_ID << "No data in buffer.";
        // TODO add logic here
        return;
    }

    ConstructMessage();
}

void ClientConnection::OnWritten(const boost::system::error_code& e) {
    if(e) {
        XERROR_WITH_ID << "Error occurred during writing to client socket, message: "
                       << e.message();
        // TODO add logic here
        return;
    }

    /// we always treat client connection as persistent connection
    reset();
    timer_.expires_from_now(timeout_);
    timer_.async_wait(std::bind(&ClientConnection::OnTimeout,
                                this, // TODO
                                std::placeholders::_1));

    read();
}

void ClientConnection::OnTimeout(const boost::system::error_code& e) {
    if (e == boost::asio::error::operation_aborted)
        XDEBUG_WITH_ID << "The client timeout timer is cancelled.";
    else if (e)
        XERROR_WITH_ID << "Error occurred with client timer, message: "
                       << e.message();
    else if (timer_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
        XDEBUG_WITH_ID << "Client socket timed out, close it.";
        timer_triggered_ = true;
        // TODO add logic here
    }
}
