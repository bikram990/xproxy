#ifndef HTTP_ENCODER_HPP
#define HTTP_ENCODER_HPP

#include "http_parser.h"

namespace x {
namespace memory { class byte_buffer; }
namespace message { class message; namespace http { class http_message; } }
namespace codec {
namespace http {

class http_encoder {
public:
    virtual std::size_t encode(const message::message& msg, memory::byte_buffer& buf);

    virtual void reset();

public:
    http_encoder(http_parser_type type)
        : type_(type), state_(BEGIN), body_encoded_(0) {}

    DEFAULT_VIRTUAL_DTOR(http_encoder);

private:
    std::size_t encode_first_line(const message::http::http_message& msg, memory::byte_buffer& buf);
    std::size_t encode_headers(const message::http::http_message& msg, memory::byte_buffer& buf);
    std::size_t encode_body(const message::http::http_message& msg, memory::byte_buffer& buf);

    enum encode_state {
        BEGIN, FIRST_LINE, HEADERS, BODY, END
    };

    http_parser_type type_;
    encode_state state_;
    std::size_t body_encoded_;

    MAKE_NONCOPYABLE(http_encoder);
};

} // namespace http
} // namespace codec
} // namespace x

#endif // HTTP_ENCODER_HPP
