#include "x/codec/http/http_decoder.hpp"
#include "x/codec/http/http_encoder.hpp"
#include "x/message/http/http_request.hpp"
#include "x/net/client_connection.hpp"
#include "x/net/connection_manager.hpp"

namespace x {
namespace net {

enum {
    // all values are measured by second
    SVR_RSP_WAITING_TIME = 15,
    IDLE_WAITING_TIME = 60
};

client_connection::client_connection(context_ptr ctx, connection_manager& mgr)
    : connection(ctx, mgr) {
    decoder_.reset(new codec::http::http_decoder(HTTP_REQUEST));
    encoder_.reset(new codec::http::http_encoder(HTTP_RESPONSE));
    message_.reset(new message::http::http_request);
    XDEBUG_WITH_ID(this) << "new client connection";
}

bool client_connection::keep_alive() {
    codec::message_decoder *decoder = decoder_.get();
    return dynamic_cast<codec::http::http_decoder *>(decoder)->keep_alive();
}

void client_connection::start() {
    connected_ = true;

    auto self(shared_from_this());
    context_->set_client_connection(self);

    read();
}

void client_connection::connect() {
    ASSERT_EXEC_RETNONE(0, stop);
}

void client_connection::handshake(ssl::certificate ca, DH *dh) {
    XDEBUG_WITH_ID(this) << "=> handshake()";

    auto callback = std::bind(&connection::on_handshake,
                              shared_from_this(),
                              std::placeholders::_1);

    socket_->switch_to_ssl(boost::asio::ssl::stream_base::server, ca, dh);
    socket_->async_handshake(callback);

    XDEBUG_WITH_ID(this) << "<= handshake()";
}

void client_connection::reset() {
    XDEBUG_WITH_ID(this) << "resetting client connection...";

    connection::reset();
    // do not reset context here, it will reset itself
    // context_->reset();
    decoder_->reset();
    encoder_->reset();
    message_->reset();

    auto self(shared_from_this());
    timer_.start(IDLE_WAITING_TIME, [self, this] (const boost::system::error_code&) {
        XERROR_WITH_ID(this) << "idle waiting timed out.";
        stop();
    });

    read();
}

void client_connection::on_connect(const boost::system::error_code&, boost::asio::ip::tcp::resolver::iterator) {
    ASSERT_EXEC_RETNONE(0, stop);
}

void client_connection::on_read(const boost::system::error_code& e, const char *data, std::size_t length) {
    XDEBUG_WITH_ID(this) << "on_read() called.";

    if (stopped_) {
        XERROR_WITH_ID(this) << "connection stopped.";
        return;
    }

    if (e) {
        if (e == boost::asio::error::eof || SSL_SHORT_READ(e)) {
            XDEBUG_WITH_ID(this) << "read, EOF in socket, stop.";
        } else {
            XERROR_WITH_ID(this) << "read error, code: " << e.value()
                                 << ", message: " << e.message();
        }
        stop();
        return;
    }

    if (length <= 0) {
        XWARN_WITH_ID(this) << "read, no data.";
        stop();
        return;
    }

    if (message_->completed()) {
        XERROR_WITH_ID(this) << "message already completed.";
        stop();
        return;
    }

    if (timer_.running())
        cancel_timer();

    if (x::log::debug_enabled()) {
        XDEBUG_WITH_ID(this) << "\n----- dump message begin -----\n"
                             << std::string(data, length)
                             << "\n------ dump message end ------";
    }

    auto consumed = decoder_->decode(data, length, *message_);
    ASSERT_EXEC_RETNONE(consumed == length, stop);

    if (!message_->deliverable()) {
        read();
        return;
    }

    auto task = [this] () { context_->on_event(READ, *this); };
    context_->service().post(task);

    auto self(shared_from_this());
    timer_.start(SVR_RSP_WAITING_TIME, [self, this] (const boost::system::error_code&) {
        XERROR_WITH_ID(this) << "server response waiting timed out.";
        stop();
    });
}

void client_connection::on_write() {
    XDEBUG_WITH_ID(this) << "on_write() called.";

    if (stopped_) {
        XERROR_WITH_ID(this) << "connection stopped.";
        return;
    }

    auto task = [this] () { context_->on_event(WRITE, *this); };
    context_->service().post(task);
}

void client_connection::on_handshake(const boost::system::error_code& e) {
    XDEBUG_WITH_ID(this) << "on_handshake() called.";

    if (stopped_) {
        XERROR_WITH_ID(this) << "connection stopped.";
        return;
    }

    CHECK_LOG_EXEC_RETURN(e, "handshake", stop);

    message_->reset();
    decoder_->reset();
    encoder_->reset();

    auto task = [this] () { context_->on_event(HANDSHAKE, *this); };
    context_->service().post(task);
}

} // namespace net
} // namespace x
