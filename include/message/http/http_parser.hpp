#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include <string>
#include "common.hpp"
#include "http_parser.h"

namespace xproxy {
namespace message {
namespace http {

class HttpMessage;

class HttpParserObserver {
public:
    DEFAULT_VIRTUAL_DTOR(HttpParserObserver);

    virtual void onHeadersComplete(HttpMessage& message) = 0;
    virtual void onBody(HttpMessage& message) = 0;
    virtual void onMessageComplete(HttpMessage& message) = 0;
};

class HttpParser {
public:
    std::size_t consume(const char* at, std::size_t length);

    bool headersCompleted() const { return headers_completed_; }

    bool messageCompleted() const { return message_completed_; }

    bool keepAlive() const;

    void reset();

public:
    HttpParser(HttpMessage& message, http_parser_type type, HttpParserObserver *observer = nullptr)
        : observer_(observer), message_(message), headers_completed_(false),
          message_completed_(false), chunked_(false) {
        ::http_parser_init(&parser_, type);
    }

    DEFAULT_VIRTUAL_DTOR(HttpParser);

    static int onMessageBegin(http_parser *parser);
    static int onUrl(http_parser *parser, const char *at, std::size_t length);
    static int onStatus(http_parser *parser, const char *at, std::size_t length);
    static int onHeaderField(http_parser *parser, const char *at, std::size_t length);
    static int onHeaderValue(http_parser *parser, const char *at, std::size_t length);
    static int onHeadersComplete(http_parser *parser);
    static int onBody(http_parser *parser, const char *at, std::size_t length);
    static int onMessageComplete(http_parser *parser);

private:
    const char *errorMessage() const {
        return ::http_errno_description(static_cast<http_errno>(parser_.http_errno));
    }

private:
    HttpParserObserver *observer_;
    HttpMessage& message_;

    bool headers_completed_;
    bool message_completed_;
    bool chunked_;
    std::string current_header_field_;
    std::string current_header_value_;

    http_parser parser_;

private:
    static http_parser_settings settings_;

    MAKE_NONCOPYABLE(HttpParser);
};

} // namespace http
} // namespace message
} // namespace xproxy

#endif // HTTP_PARSER_HPP
