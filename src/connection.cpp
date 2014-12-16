#include "x/codec/http/http_decoder.hpp"
#include "x/codec/http/http_encoder.hpp"
#include "x/message/http/http_request.hpp"
#include "x/message/http/http_response.hpp"
#include "x/net/connection.hpp"

namespace x {
namespace net {

connection::connection(context_ptr ctx)
    : connected_(false),
      stopped_(false),
      socket_(new socket_wrapper(ctx->service())),
      timer_(ctx->service()),
      context_(ctx),
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
        CHECK_RETURN(e, "read");

        auto consumed = decoder_->decode(buffer_in_.data(), length, *message_);
        assert(consumed == length);

        if (message_->deliverable()) {
            on_read();
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

    // notify the bridged connection(if it exists)
    #warning add code here
    // auto session = session_.lock();
    // if (session)
    //     session->on_connection_stop(shared_from_this());

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
        CHECK_RETURN(e, "write");

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
        on_write();
    });

    XDEBUG_WITH_ID(this) << "<= do_write()";
}

} // namespace net
} // namespace x
