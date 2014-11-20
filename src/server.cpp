#include "x/conf/config.hpp"
#include "x/log/log.hpp"
#include "x/net/connection.hpp"
#include "x/net/server.hpp"
#include "x/ssl/certificate_manager.hpp"

namespace x {
namespace net {

server::server()
    : signals_(service_),
      acceptor_(service_),
      config_(new x::conf::config),
      cert_manager_(new x::ssl::certificate_manager) {}

bool server::init() {
    if (!config_->load_config()) {
        XFATAL << "unable to load server configuration.";
        return false;
    }

    if (!cert_manager_->init()) {
        XFATAL << "unable to init certificate manager.";
        return false;
    }

    if (!config_->get_config("basic.port", port_))
        port_ = DEFAULT_SERVER_PORT;

    init_signal_handler();
    init_acceptor();
}

void server::start() {
    start_accept();
}

void server::init_signal_handler() {
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
#warning is the following needed?
    // signals_.add(SIGQUIT);
    signals_.async_wait([this] (const boost::system::error_code&, int) {
        XINFO << "stopping xProxy...";
        acceptor_.close();
    });
}

void server::init_acceptor() {
    using namespace boost::asio::ip;

    tcp::endpoint e(tcp::v4(), port_);
    acceptor_.open(e.protocol());
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
    acceptor_.bind(e);
    acceptor_.listen();
}

void server::start_accept() {
    context_ptr ctx(service_);
    current_connection_.reset(new client_connection(ctx));
    acceptor_.async_accept(current_connection_->socket(),
                           [this] (const boost::system::error_code& e) {
        if (e) {
            XERROR << "accept error, code: " << e.value()
                   << ", message: " << e.message();
            return;
        }

        auto addr = current_connection_->socket().remote_endpoint().address();
        auto port = current_connection_->socket().remote_endpoint().port();
        current_connection_->set_host(addr.to_string());
        current_connection_->set_port(port);

        XDEBUG << "new client connection, id: " << current_connection_->id()
               << ", addr: " << addr << ", port: " << port;

        current_connection_->start();

        start_accept();
    });
}

} // namespace net
} // namespace x
