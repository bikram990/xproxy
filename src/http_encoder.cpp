#include <map>
#include <string>
#include "x/codec/http/http_encoder.hpp"
#include "x/common.hpp"
#include "x/memory/byte_buffer.hpp"
#include "x/message/message.hpp"
#include "x/message/http/http_message.hpp"
#include "x/message/http/http_request.hpp"
#include "x/message/http/http_response.hpp"

namespace x {
namespace codec {
namespace http {

std::size_t http_encoder::encode(const message::message& msg, memory::byte_buffer& buf) {
    message::http::http_message *message = dynamic_cast<message::http::http_message *>(&msg);
    assert(message);

    // we do not do encoding if the headers are not completed
    if (!message->headers_completed())
        return 0;

    switch (state_) {
    case BEGIN: {
        auto line_length = encode_first_line(message, buf);
        auto headers_length = encode_headers(message, buf);
        auto body_length= encode_body(message, buf);
        return line_length + headers_length + body_length;
    }
    case FIRST_LINE: {
        auto headers_length = encode_headers(message, buf);
        auto body_length= encode_body(message, buf);
        return headers_length + body_length;
    }
    case HEADERS: {
        auto body_length = encode_body(message, buf);
        return body_length;
    }
    case BODY: {
        auto body_length = encode_body(message, buf);
        return body_length;
    }
    case END: {
        return 0;
    }
    default:
        assert(0);
    }
}

std::size_t http_encoder::encode_first_line(const message::http::http_message& msg, memory::byte_buffer& buf) {
    assert(msg.headers_completed());
    assert(state_ == BEGIN);

    std::string first_line;

    if (type_ == HTTP_REQUEST) {
        message::http::http_request *request = dynamic_cast<message::http::http_request *>(&msg);
        assert(request);

        first_line.append(request->get_method())
                .append(' ')
                .append(request->get_uri())
                .append(' ')
                .append("HTTP/")
                .append(request->get_major_version())
                .append('.')
                .append(request->get_minor_version())
                .append(CRLF);
    } else {
        message::http::http_response *response = dynamic_cast<message::http::http_response *>(&msg);
        assert(response);

        first_line.append("HTTP/")
                .append(response->get_major_version())
                .append('.')
                .append(response->get_minor_version())
                .append(' ')
                .append(response->get_status())
                .append(' ')
                .append(response->get_message())
                .append(CRLF);
    }

    buf << first_line;

    state_ = FIRST_LINE;
    return first_line.length();
}

std::size_t http_encoder::encode_headers(const message::http::http_message& msg, memory::byte_buffer& buf) {
    assert(msg.headers_completed());
    assert(state_ == FIRST_LINE);

    auto orig_size = buf.size();

    auto& headers = msg.get_headers();
    for (auto it = std::begin(headers); it != std::end(headers); ++it) {
        buf << it.first << ": " << it.second << CRLF;
    }
    buf << CRLF;

    state_ = HEADERS;
    return buf.size() - orig_size;
}

std::size_t http_encoder::encode_body(const message::http::http_message& msg, memory::byte_buffer& buf) {
    assert(msg.headers_completed());
    assert(state_ == HEADERS || state_ == BODY);
    if (state_ == HEADERS)
        assert(body_encoded_ == 0);
    else
        assert(body_encoded_ > 0);

    auto& body = msg.get_body();
    auto inc = body.size() - body_encoded_;
    buf << memory::byte_buffer::wrap(body.data(body_encoded_), inc);
    body_encoded_ += inc;

    if (msg.message_completed())
        state_ = END;
    else
        state_ = BODY;

    return inc;
}

} // namespace http
} // namespace codec
} // namespace x
