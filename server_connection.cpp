#include <boost/lexical_cast.hpp>
#include "http_response.h"
#include "log.h"
#include "server_connection.h"
#include "session.h"

ServerConnection::ServerConnection(std::shared_ptr<Session> session)
    : Connection(session, 15), resolver_(session->service()) { // TODO
    message_.reset(new HttpResponse);
}

ServerConnection::~ServerConnection() {}

void ServerConnection::connect() {
    // we switch to https mode before we connect to server
//    if(context_.https) {
//        server_socket_->SwitchProtocol<ResourceManager::CertManager::CAPtr,
//                ResourceManager::CertManager::DHParametersPtr>(kHttps);
//    }

    try {
        boost::asio::ip::tcp::resolver::query query(host_, boost::lexical_cast<std::string>(port_));
        auto endpoint_iterator = resolver_.resolve(query);

        XDEBUG_WITH_ID << "Connecting to remote, host: " << host_
                       << ", port: " << port_ << ", resolved address: "
                       << endpoint_iterator->endpoint().address();

        socket_->async_connect(endpoint_iterator,
                               std::bind(&ServerConnection::OnConnected,
                                         this, // TODO
                                         std::placeholders::_1));
        } catch(const boost::system::system_error& e) {
            XERROR_WITH_ID << "Failed to resolve [" << host_ << ":" << port_ << "], error: " << e.what();
            // TODO add logic here
        }
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

void ServerConnection::OnConnected(const boost::system::error_code& e) {
    if (e) {
        XERROR_WITH_ID << "Error occurred during connecting to remote peer, message: "
                       << e.message();
        // TODO add logic here
        return;
    }

    XDEBUG_WITH_ID << "Remote peer connected: " << host_ << ":" << port_;
    connected_ = true;
    write();
}

void ServerConnection::NewDataCallback(std::shared_ptr<Session> session) {
    service_.post(std::bind(&Session::OnResponse, session, message_));
}

void ServerConnection::CompleteCallback(std::shared_ptr<Session> session) {
    service_.post(std::bind(&Session::OnResponseComplete, session, message_));
}
