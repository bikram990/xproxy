#include "x/codec/http/http_decoder.hpp"
#include "x/codec/http/http_encoder.hpp"
#include "x/message/http/http_request.hpp"
#include "x/net/client_connection.hpp"

namespace x {
namespace net {

client_connection::client_connection(context_ptr ctx)
    : connection(ctx) {
    decoder_.reset(new codec::http::http_decoder(HTTP_REQUEST));
    encoder_.reset(new codec::http::http_encoder(HTTP_RESPONSE));
    message_.reset(new message::http::http_request);
    XDEBUG_WITH_ID(this) << "new client connection";
}

void client_connection::start() {
    connected_ = true;
    context_->set_client_connection(shared_from_this());
    read();
}

void client_connection::connect() {
    assert(0);
}

void client_connection::handshake(ssl::certificate ca, DH *dh) {
    XDEBUG_WITH_ID(this) << "=> handshake()";

    auto self(shared_from_this());
    socket_->switch_to_ssl(boost::asio::ssl::stream_base::client, ca, dh);
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

void client_connection::on_connect() {
    assert(0);
}

void client_connection::on_read() {
    context_->on_event(READ, *this);
    auto self(shared_from_this());
    timer_.start(30, [self, this] (const boost::system::error_code&) {
        XERROR_WITH_ID(this) << "waiting for server response timed out.";
        stop();
    });
}

void client_connection::on_write() {
    context_->on_event(WRITE, *this);
}

void client_connection::on_handshake() {
    message_->reset();
    decoder_->reset();

    read();
}

} // namespace net
} // namespace x
