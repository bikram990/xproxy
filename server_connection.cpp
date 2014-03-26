#include "http_response.h"
#include "log.h"
#include "server_connection.h"
#include "session.h"

ServerConnection::ServerConnection(std::shared_ptr<Session> session)
    : Connection(session, 15){} // TODO

ServerConnection::~ServerConnection() {}

void ServerConnection::InitMessage() {
    message_.reset(new HttpResponse);
}

void ServerConnection::OnRead(const boost::system::error_code& e) {
    if (timer_triggered_) {
        XDEBUG_WITH_ID << "Server socket timed out, abort reading.";
        timer_triggered_ = false;
        return;
    }

    //if(server_.state() == ProxyServer::kStopped) {
    if (false) { // TODO
        XDEBUG_WITH_ID << "xProxy server is stopping, abort reading from server.";
        timer_.cancel();
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
            // TODO add logic here
            return;
        }
    }

    if(buffer_in_.size() <= 0) {
        XWARN_WITH_ID << "No data in server socket.";
        // TODO add logic here
        return;
    }

    ConstructMessage();
}

void ServerConnection::OnWritten(const boost::system::error_code& e) {
    // if(server_.state() == ProxyServer::kStopped) {
    if (false) { // TODO
        XDEBUG_WITH_ID << "xProxy server is stopping, abort writing to server.";
        timer_.cancel();
        return;
    }

    if (e) {
        XERROR_WITH_ID << "Error occurred during writing to remote peer, message: "
                       << e.message();
        // TODO add logic here
        return;
    }

    XDEBUG_WITH_ID << "Data has been written to remote peer, now start reading...";

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

void ServerConnection::NewDataCallback(std::shared_ptr<Session> session) {
    service_.post(std::bind(&Session::OnResponse, session, message_));
}

void ServerConnection::CompleteCallback(std::shared_ptr<Session> session) {
    service_.post(std::bind(&Session::OnResponseComplete, session, message_));
}
