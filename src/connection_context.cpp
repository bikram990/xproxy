#include <boost/lexical_cast.hpp>
#include "x/net/client_connection.hpp"
#include "x/net/connection.hpp"
#include "x/net/connection_context.hpp"
#include "x/net/server.hpp"
#include "x/net/server_connection.hpp"
#include "x/message/http/http_request.hpp"
#include "x/message/http/http_response.hpp"

namespace x {
namespace net {

boost::asio::io_service& connection_context::service() const {
    return server_.get_service();
}

void connection_context::reset() {
    message_exchange_completed_= false;
    state_ = READY;
}

void connection_context::on_event(connection_event event, client_connection& conn) {
    switch (event) {
    case READ:
        return on_client_message(conn.get_message());
    case WRITE: {
        if (state_ == CLIENT_SSL_REPLYING) {
            auto svr_conn(server_conn_.lock());
            assert(svr_conn);
            auto& cert_mgr = server_.get_certificate_manager();
            conn.handshake(cert_mgr.get_certificate(svr_conn->get_host()), cert_mgr.get_dh_parameters());
            return;
        }

        if (message_exchange_completed_) {
            if (conn.keep_alive()) {
                XDEBUG << "response completed, keep client connection [id: " << conn.id() << "] alive.";
                conn.reset();
                // as when client's on_write is invoked, the server connection
                // is already reset earlier, so we reset the context itself here
                reset();
            } else {
                XDEBUG << "response completed, close client connection [id: " << conn.id() << "].";
                conn.stop();
            }
        }

        return;
    }
    default:
        assert(0);
    }
}

void connection_context::on_event(connection_event event, server_connection& conn) {
    switch (event) {
    case CONNECT: {
        if (https_) {
            conn.handshake();
            return;
        }

        conn.write();
        return;
    }
    case READ:
        return on_server_message(conn.get_message());
    case HANDSHAKE: {
        conn.write();
        return;
    }
    default:
        assert(0);
    }
}

void connection_context::on_client_message(message::message& msg) {
    assert(state_ == READY || state_ == CLIENT_SSL_REPLYING);
    auto request = dynamic_cast<message::http::http_request *>(&msg);
    assert(request);
    assert(request->completed());

    if (state_ == READY) {
        auto svr_conn(server_conn_.lock());

        if (!svr_conn) {
            std::string host;
            unsigned short port;
            parse_destination(*request, https_, host, port);

            svr_conn = std::make_shared<server_connection>(shared_from_this(),
                                                           server_.get_server_connection_manager());
            svr_conn->set_host(host);
            svr_conn->set_port(port);
            server_conn_ = svr_conn;
        }


        if (!https_) {
            svr_conn->write(msg);
            state_ = SERVER_WRITING;
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
    auto client_conn(client_conn_.lock());
    auto server_conn(server_conn_.lock());
    assert(client_conn);
    assert(server_conn);

    client_conn->write(msg);

    if (!msg.completed()) {
        server_conn->read();
        return;
    }

    message_exchange_completed_ = true;

    if (server_conn->keep_alive()) {
        XDEBUG << "response completed, keep server connection [id: " << server_conn->id() << "] alive.";
        server_conn->reset();
    } else {
        server_conn->stop();
        XDEBUG << "response completed, close server connection [id: " << server_conn->id() << "].";
    }
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
