#include "x/net/connection.hpp"
#include "x/net/connection_context.hpp"

namespace x {
namespace net {

void connection_context::on_client_message(message::message& msg) {
    #warning implement logic here

    // check the server connection first
    auto tmp(server_conn_.lock());
    assert(!tmp);

    connection_ptr svr_conn(new server_connection(shared_from_this()));
    server_conn_ = svr_conn;

    svr_conn->start();
}

void connection_context::on_server_message(message::message& msg) {
    #warning implement logic here
}

} // namespace net
} // namespace x
