#include "xproxy/net/socket_facade.hpp"
#include "xproxy/proxy_server.hpp"

namespace xproxy {

ProxyServer::ProxyServer(unsigned short port)
    : port_(port),
      signals_(service_),
      acceptor_(service_) {}

void ProxyServer::start() {
    init();
    startAccept();
    XINFO << "xProxy is listening at port " << port_
          << ", Press Ctrl-C to stop.";
    service_.run();
}

void ProxyServer::init() {
    using namespace boost::asio::ip;

    signals_.add(SIGINT);
    signals_.add(SIGTERM);
    // signals_.add(SIGQUIT); // TODO is this needed?
    signals_.async_wait([this] (const boost::system::error_code&, int) {
        XINFO << "Stopping xProxy...";
        acceptor_.close();
        client_connection_manager_.stopAll();
    });

    tcp::endpoint e(tcp::v4(), port_);
    acceptor_.open(e.protocol());
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
    acceptor_.bind(e);
    acceptor_.listen();
}

void ProxyServer::startAccept() {
    current_client_connection_ = net::createBridgedConnections(service_, &client_connection_manager_, nullptr);
    acceptor_.async_accept(current_client_connection_->socket(),
                           [this] (const boost::system::error_code& e) {
        if (e) {
            XERROR << "Accept error: " << e.message();
            return;
        }

        XDEBUG << "New client connection[id: "
               << current_client_connection_->id()
               << "] address: ["
               << current_client_connection_->socket().remote_endpoint().address()
               << ':'
               << current_client_connection_->socket().remote_endpoint().port()
               << "].";

        client_connection_manager_.start(current_client_connection_);

        startAccept();
    });
}

} // namespace xproxy
