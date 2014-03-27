#ifndef HTTP_MESSAGE_H
#define HTTP_MESSAGE_H

#include "common.h"
#include "http_headers.h"
#include "http_parser.h"

class HttpMessage {
public:
    bool consume(boost::asio::streambuf& buffer);

    virtual bool serialize(boost::asio::streambuf& out_buffer) = 0;

    virtual void reset();

    bool HeaderCompleted() const { return header_completed_; }

    bool MessageCompleted() const { return message_completed_; }

    int MajorVersion() const { return parser_.http_major; }

    int MinorVersion() const { return parser_.http_minor; }

    const HttpHeaders& headers() const { return headers_; }

private:
    static int MessageBeginCallback(http_parser *parser);
    static int UrlCallback(http_parser *parser, const char *at, std::size_t length);
    static int StatusCallback(http_parser *parser, const char *at, std::size_t length);
    static int HeaderFieldCallback(http_parser *parser, const char *at, std::size_t length);
    static int HeaderValueCallback(http_parser *parser, const char *at, std::size_t length);
    static int HeadersCompleteCallback(http_parser *parser);
    static int BodyCallback(http_parser *parser, const char *at, std::size_t length);
    static int MessageCompleteCallback(http_parser *parser);

protected:
    HttpMessage(http_parser_type type);
    virtual ~HttpMessage() = default;

    // this method should be overridden in HttpRequest
    virtual int OnUrl(const char *at, std::size_t length) { return 0; }

    // this method should be overridden in HttpResponse
    virtual int OnStatus(const char *at, std::size_t length) { return 0; }

protected:
    http_parser parser_;

    bool header_completed_;
    bool message_completed_;
    std::string current_header_field_; // store temp header name/value during parsing
    std::string current_header_value_;

    HttpHeaders headers_;
    boost::asio::streambuf body_;

private:
    static http_parser_settings settings_;

private:
    DISABLE_COPY_AND_ASSIGNMENT(HttpMessage);
};

#endif // HTTP_MESSAGE_H
