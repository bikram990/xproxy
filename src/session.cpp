#include "x/net/connection.hpp"
#include "x/net/server.hpp"
#include "x/net/session.hpp"
#include "x/net/session_manager.hpp"

namespace x {
namespace net {

session::session(server &server)
    : service_(server.get_service()),
      config_(server.get_config()),
      session_manager_(server.get_session_manager()),
      cert_manager_(server.get_certificate_manager()) {}

void session::start() {
    client_connection_.reset(new client_connection(shared_from_this()));
    server_connection_.reset(new server_connection(shared_from_this()));

    client_connection_->start();
}

void session::stop() {
    session_manager_.erase(shared_from_this());
#warning add more code here
}

void session::on_connection_stop(conn_ptr connection) {
    // 1. remove from manager
    session_manager_.erase(shared_from_this());

    // 2. notify the other side
    if (client_connection_ == connection)
        server_connection_->stop();
    else if (server_connection_ == connection)
        client_connection_->stop();
    else
        assert(0);
}

} // namespace net
} // namespace x
