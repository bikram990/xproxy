#include "x/codec/message_decoder.hpp"
#include "x/codec/message_encoder.hpp"
#include "x/handler/message_handler.hpp"
#include "x/memory/byte_buffer.hpp"
#include "x/message/message.hpp"
#include "x/net/connection.hpp"
#include "x/net/session.hpp"

namespace x {
namespace net {

connection::connection(session_ptr session)
    : connected_(false),
      stopped_(false),
      socket_(new socket_wrapper(session->get_service())),
      session_(session),
      #warning add more constructions here
      writing_(false) {}

void connection::read() {
    XDEBUG_WITH_ID(this) << "=> read()";

    assert(connected_);
    assert(!stopped_);

    auto self(shared_from_this());
    boost::asio::async_read(*socket_,
                            boost::asio::buffer(buffer_in_),
                            boost::asio::transfer_at_least(1),
                            [self, this] (const boost::system::error_code& e, std::size_t length) {
        if (e) {
            XERROR_WITH_ID(this) << "read error, code: " << e.value()
                                 << ", message: " << e.message();
            stop();
            return;
        }

        auto consumed = decoder_->decode(buffer_in_.data(), length, *message_);
        assert(consumed == length);

        if (message_->deliverable()) {
            handler_->handle_message(*message_);
            return;
        }

        read();
    });

    XDEBUG_WITH_ID(this) << "<= read()";
}

void connection::write(const message::message &message) {
    XDEBUG_WITH_ID(this) << "=> write()";

    memory::buffer_ptr buf(new memory::byte_buffer);
    encoder_->encode(message, *buf);

    buffer_out_.push_back(buf);
    do_write();

    XDEBUG_WITH_ID(this) << "<= write()";
}

void connection::stop() {
    if (stopped_) {
        XWARN_WITH_ID(this) << "connection already stopped.";
        return;
    }

    XDEBUG_WITH_ID(this) << "stopping connection...";

    // cancel timer
    #warning add code here

    // close socket
    if (connected_ && socket_) {
        socket_->close();
        connected_ = false;
    }

    // notify session (if it exists)
    auto session = session_.lock();
    if (session)
        session->on_connection_stop(shared_from_this());

    stopped_ = true;
}

void connection::do_write() {
    if (writing_) return;
    if (buffer_out_.empty()) return;

    XDEBUG_WITH_ID(this) << "=> do_write()";

    writing_ = true;
    auto& candidate = buffer_out_.front();
    auto self(shared_from_this());
    socket_->async_write_some(boost::asio::buffer(candidate->data(), candidate->size()),
                                                  [self, candidate, this] (const boost::system::error_code& e, std::size_t length) {
        if (e) {
            XERROR_WITH_ID(this) << "write error, code: " << e.value()
                                 << ", message: " << e.message();
            stop();
            return;
        }

        writing_ = false;
        if (length < candidate->size()) {
            XERROR_WITH_ID(this) << "write incomplete, continue.";
            candidate->erase(0, length);
            do_write();
            return;
        }

        buffer_out_.erase(buffer_out_.begin());
        if (!buffer_out_.empty()) {
            XDEBUG_WITH_ID(this) << "more buffers added, continue.";
            do_write();
            return;
        }

#warning add something here
    });

    XDEBUG_WITH_ID(this) << "<= do_write()";
}

client_connection::client_connection(session_ptr session)
    : connection(session) {}

void client_connection::start() {
    connected_ = true;
    host_ = socket_->socket().remote_endpoint().address().to_string();
    port_ = socket_->socket().remote_endpoint().port();
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
#warning do something here
    });

    XDEBUG_WITH_ID(this) << "<= handshake()";
}

server_connection::server_connection(session_ptr session)
    : connection(session),
      resolver_(session->get_service()) {}

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
        });
    });

    XDEBUG_WITH_ID(this) << "<= connect()";
}

void server_connection::handshake(ssl::certificate ca, DH *dh) {
    XDEBUG_WITH_ID(this) << "=> handshake()";

    auto self(shared_from_this());
    socket_->switch_to_ssl(boost::asio::ssl::stream_base::server, ca, dh);
    socket_->async_handshake([self, this] (const boost::system::error_code& e) {
#warning do something here
    });

    XDEBUG_WITH_ID(this) << "<= handshake()";
}

} // namespace net
} // namespace x
