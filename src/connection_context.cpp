#include "x/common.hpp"
#include "x/net/connection.hpp"
#include "x/net/connection_context.hpp"

namespace x {
namespace net {

void connection_context::on_message(message::message& msg) {
    #warning correct the logic here, the msg could be a server message

    // check the server connection first
    auto tmp(server_conn_.lock());
    assert(!tmp);

    conn_ptr svr_conn(new server_connection(shared_from_this()));
    server_conn_ = svr_conn;

    svr_conn->start();
}

} // namespace net
} // namespace x
