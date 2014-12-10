#include <boost/lexical_cast.hpp>
#include "x/net/connection.hpp"
#include "x/net/connection_context.hpp"
#include "x/message/http/http_request.hpp"
#include "x/message/http/http_response.hpp"

namespace x {
namespace net {

void connection_context::on_client_message(message::message& msg) {
    #warning implement logic here

    assert(state_ == READY || state_ == CLIENT_HANDSHAKE);

    auto tmp(server_conn_.lock());
    if (state_ == READY) {
        assert(!tmp);
    } else {
        assert(tmp);
        assert(https_);
    }

    message::http::http_request *request = dynamic_cast<message::http::http_request *>(&msg);
    assert(request);
    assert(request->message_completed());

    std::string host;
    unsigned short port;

    auto method = request->get_method();
    if (method.length() == 7 && method[0] == 'C' && method[1] == 'O') {
        https_ = true;
        host = request->get_uri();
        port = 443;
    } else {
        auto found = request->find_header("Host", host);
        assert(found);
        port = 80;
    }

    auto sep = host.find(':');
    if (sep != std::string::npos) {
        port = boost::lexical_cast<unsigned short>(host.substr(sep + 1));
        host = host.substr(0, sep);
    }

    connection_ptr svr_conn(new server_connection(shared_from_this()));
    server_conn_ = svr_conn;

    svr_conn->set_host(host);
    svr_conn->set_port(port);

    svr_conn->start();
}

void connection_context::on_server_message(message::message& msg) {
    #warning implement logic here
}

} // namespace net
} // namespace x
