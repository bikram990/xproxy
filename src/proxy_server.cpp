#include "client_connection.h"
#include "log.h"
#include "proxy_server.h"
#include <functional>

ProxyServer::ProxyServer(unsigned short port)
    : port_(port), signals_(service_), acceptor_(service_) {}

void ProxyServer::start() {
    if(!init()) {
        XFATAL << "Failed to initialzie proxy server.";
        return;
    }

    StartAccept();
    XINFO << "Proxy is started.";
}

bool ProxyServer::init() {
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
    // signals_.add(SIGQUIT); // TODO is this needed?
    signals_.async_wait(std::bind(&ProxyServer::OnStopSignalReceived, this));

    boost::asio::ip::tcp::endpoint e(boost::asio::ip::tcp::v4(), port_);
    acceptor_.open(e.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(e);
    acceptor_.listen();

    return true;
}

void ProxyServer::StartAccept() {
    current_client_connection_.reset(new ClientConnection(*this));
    acceptor_.async_accept(current_client_connection_->socket(),
                           std::bind(&ProxyServer::OnConnectionAccepted, this, std::placeholders::_1));
}

void ProxyServer::OnConnectionAccepted(const boost::system::error_code& e) {
    if(e) {
        XERROR << "Error occurred during accept connection, message: " << e.message();
        return;
    }

    XDEBUG << "A new client connection [id: " << current_client_connection_->id()
           << "] is established, client address: ["
           << current_client_connection_->socket().remote_endpoint().address() << ":"
           << current_client_connection_->socket().remote_endpoint().port() << "].";

    client_connections_.insert(current_client_connection_);
    current_client_connection_->read();

    StartAccept();
}

void ProxyServer::OnStopSignalReceived() {
    XINFO << "xProxy server is stopping...";
    acceptor_.close();
    StopAllConnections();
}

void ProxyServer::StopAllConnections() {
    std::for_each(client_connections_.begin(),
                  client_connections_.end(),
                  [](std::shared_ptr<Connection> connection) { connection->stop(); });
    client_connections_.clear();
}
