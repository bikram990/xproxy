#include <boost/lexical_cast.hpp>
#include "x/net/connection.hpp"
#include "x/net/connection_context.hpp"
#include "x/message/http/http_request.hpp"
#include "x/message/http/http_response.hpp"

namespace x {
namespace net {

void connection_context::on_client_message(message::message& msg) {
    assert(state_ == READY || state_ == CLIENT_SSL_REPLYING);
    auto request = dynamic_cast<message::http::http_request *>(&msg);
    assert(request);
    assert(request->message_completed());

    if (state_ == READY) {
        auto tmp(server_conn_.lock());
        assert(!tmp);

        std::string host;
        unsigned short port;
        parse_destination(*request, https_, host, port);

        auto svr_conn = std::make_shared<server_connection>(shared_from_this());
        svr_conn->set_host(host);
        svr_conn->set_port(port);

        server_conn_ = svr_conn;

        if (!https_) {
            svr_conn->start();
            state_ = SERVER_CONNECTING;
        } else{
            auto client_conn(client_conn_.lock());
            assert(client_conn);

            using namespace message::http;
            auto response = http_response::make_response(http_response::SSL_REPLY);
            client_conn->write(*response);
            state_ = CLIENT_SSL_REPLYING;
        }
    } else {
        auto svr_conn(server_conn_.lock());
        assert(svr_conn);

        svr_conn->write(msg);
        state_ = SERVER_WRITING;
    }
}

void connection_context::on_server_message(message::message& msg) {
#warning implement logic here
}

void connection_context::parse_destination(const message::http::http_request &request,
                                           bool& https, std::string& host, unsigned short& port) {
    auto method = request.get_method();
    if (method.length() == 7 && method[0] == 'C' && method[1] == 'O') {
        https = true;
        host = request.get_uri();
        port = 443;
    } else {
        auto found = request.find_header("Host", host);
        assert(found);
        port = 80;
    }

    auto sep = host.find(':');
    if (sep != std::string::npos) {
        port = boost::lexical_cast<unsigned short>(host.substr(sep + 1));
        host = host.substr(0, sep);
    }
}

} // namespace net
} // namespace x
