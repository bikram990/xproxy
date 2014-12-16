#include "x/codec/http/http_decoder.hpp"
#include "x/codec/http/http_encoder.hpp"
#include "x/message/http/http_response.hpp"
#include "x/net/server_connection.hpp"

namespace x {
namespace net {

server_connection::server_connection(context_ptr ctx)
    : connection(ctx),
      resolver_(ctx->service()) {
    decoder_.reset(new codec::http::http_decoder(HTTP_RESPONSE));
    encoder_.reset(new codec::http::http_encoder(HTTP_REQUEST));
    message_.reset(new message::http::http_response);
    XDEBUG_WITH_ID(this) << "new server connection";
}

void server_connection::start() {
    assert(host_.length() > 0);
    assert(port_ != 0);

    connect();
}

void server_connection::connect() {
    XDEBUG_WITH_ID(this) << "=> connect()";

    using namespace boost::asio::ip;
    auto self(shared_from_this());
    resolver_.async_resolve(tcp::resolver::query(host_, std::to_string(port_)),
                            [self, this] (const boost::system::error_code& e, tcp::resolver::iterator it) {
        if (e) {
            XERROR_WITH_ID(this) << "resolve error, code: " << e.value()
                                 << ", message: " << e.message();
            stop();
            return;
        }

        socket_->async_connect(it, [self, this] (const boost::system::error_code& e, tcp::resolver::iterator it) {
            if (e) {
                XERROR_WITH_ID(this) << "connect error, code: " << e.value()
                                     << ", message: " << e.message();
                stop();
                return;
            }

            connected_ = true;
#warning add something here
            on_connect();
        });
    });

    XDEBUG_WITH_ID(this) << "<= connect()";
}

void server_connection::handshake(ssl::certificate ca, DH *dh) {
    XDEBUG_WITH_ID(this) << "=> handshake()";

    auto self(shared_from_this());
    socket_->switch_to_ssl(boost::asio::ssl::stream_base::server, ca, dh);
    socket_->async_handshake([self, this] (const boost::system::error_code& e) {
        if (e) {
            XERROR_WITH_ID(this) << "handshake error, code: " << e.value()
                                 << ", message: " << e.message();
            stop();
            return;
        }

#warning do something here
        on_handshake();
    });

    XDEBUG_WITH_ID(this) << "<= handshake()";
}

void server_connection::on_connect() {
    context_->on_event(CONNECT, *this);
}

void server_connection::on_read() {
    context_->on_event(READ, *this);
}

void server_connection::on_write() {
    read();
}

void server_connection::on_handshake() {
    context_->on_event(HANDSHAKE, *this);
}

} // namespace net
} // namespace x
