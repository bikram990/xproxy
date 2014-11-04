#include "x/net/server.hpp"
#include "x/conf/config.hpp"
#include "x/net/session.hpp"
#include "x/net/session_manager.hpp"
#include "x/ssl/certificate_manager.hpp"

namespace x {
namespace net {

server::server()
    : signals_(service_),
      acceptor_(service_),
      config_(new x::conf::config),
      session_manager_(new session_manager),
      cert_manager_(new x::ssl::certificate_manager) {}

bool server::init() {
    if (!conf.load_config()) {
        XFATAL << "unable to load server configuration.";
        return false;
    }

    if (!cert_manager_.init()) {
        XFATAL << "unable to init certificate manager.";
        return false;
    }

    if (!config_.get_config("basic.port", port_))
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
        session_manager_.stop_all();
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
    current_session_ = new session(service_);
    conn_ptr client_conn = current_session_->get_connection(session::CLIENT_SIDE);
    acceptor_.async_accept(client_conn->socket(),
                           [this] (const boost::system::error_code& e) {
        if (e) {
            XERROR << "accept error, code: " << e.value()
                   << ", message: " << e.message();
            return;
        }

        XTRACE << "new client connection, id: " << client_conn->id()
               << ", addr: " << client_conn->socket().remote_endpoint().address()
               << ", port: " << client_conn->socket().remote_endpoint().port();

        session_manager_.start(current_session_);

        start_accept();
    });
}

} // namespace net
} // namespace x
