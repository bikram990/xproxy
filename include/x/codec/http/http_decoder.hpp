#ifndef HTTP_DECODER_HPP
#define HTTP_DECODER_HPP

#include "http_parser.h"
#include "x/common.hpp"
#include "x/message/http/http_message.hpp"

namespace x {
namespace codec {
namespace http {

class http_decoder {
public:
    virtual std::size_t decode(const char *begin, std::size_t length, message::message& msg);

    virtual void reset();

    bool headersCompleted() const { return headers_completed_; }

    bool messageCompleted() const { return message_completed_; }

    bool keepAlive() const;

    void reset();

public:
    http_decoder(message::http::http_message& message)
        : message_(message), headers_completed_(false),
          message_completed_(false), chunked_(false) {
        ::http_parser_init(&parser_, message_.get_decoder_type());
        parser_.data = this;
    }

    DEFAULT_VIRTUAL_DTOR(http_decoder);

    static int on_message_begin(http_parser *parser);
    static int on_url(http_parser *parser, const char *at, std::size_t length);
    static int on_status(http_parser *parser, const char *at, std::size_t length);
    static int on_header_field(http_parser *parser, const char *at, std::size_t length);
    static int on_header_value(http_parser *parser, const char *at, std::size_t length);
    static int on_headers_complete(http_parser *parser);
    static int on_body(http_parser *parser, const char *at, std::size_t length);
    static int on_message_complete(http_parser *parser);

private:
    const char *error_message() const {
        return ::http_errno_description(static_cast<http_errno>(parser_.http_errno));
    }

private:
    message::http::http_message& message_;

    bool headers_completed_;
    bool message_completed_;
    bool chunked_;
    std::string current_header_field_;
    std::string current_header_value_;

    http_parser parser_;

private:
    static http_parser_settings settings_;

    MAKE_NONCOPYABLE(http_decoder);
};

} // namespace http
} // namespace codec
} // namespace x

#endif // HTTP_DECODER_HPP
