#include "http_response.h"

boost::asio::streambuf& HttpResponse::OutboundBuffer() {
    if(buffer_built_)
        return raw_buffer_;

    std::ostream buf(&raw_buffer_);

    buf << status_line_ << "\r\n";

    for(std::vector<HttpHeader>::iterator it = headers_.begin();
        it != headers_.end(); ++it)
        buf << it->name << ": " << it->value << "\r\n";

    buf << "\r\n";

    if(body_.size() > 0) {
        boost::asio::streambuf::mutable_buffers_type in = raw_buffer_.prepare(body_.size());
        std::size_t copied = boost::asio::buffer_copy(in, body_.data());
        raw_buffer_.commit(copied);
    }

    buffer_built_ = true;
    return raw_buffer_;
}
