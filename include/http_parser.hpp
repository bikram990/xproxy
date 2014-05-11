#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include "common.h"
#include "http_parser.h"

class ConnectionAdapter;
class HttpMessage;

class HttpParser {
public:
    std::size_t consume(const char* at, std::size_t length);

    bool HeaderCompleted() const { return header_completed_; }

    bool MessageCompleted() const { return message_completed_; }

    bool KeepAlive() const;

    void reset();

public:
    HttpParser(ConnectionAdapter *adapter, HttpMessage& message, http_parser_type type)
        : adapter_(adapter), message_(message), header_completed_(false),
          message_completed_(false), chunked_(false) {
        ::http_parser_init(&parser_, type);
    }

    virtual ~HttpParser() = default;

    static int OnMessageBegin(http_parser *parser);
    static int OnUrl(http_parser *parser, const char *at, std::size_t length);
    static int OnStatus(http_parser *parser, const char *at, std::size_t length);
    static int OnHeaderField(http_parser *parser, const char *at, std::size_t length);
    static int OnHeaderValue(http_parser *parser, const char *at, std::size_t length);
    static int OnHeadersComplete(http_parser *parser);
    static int OnBody(http_parser *parser, const char *at, std::size_t length);
    static int OnMessageComplete(http_parser *parser);

private:
    ConnectionAdapter *adapter_;
    HttpMessage& message_;

    bool header_completed_;
    bool message_completed_;
    bool chunked_;
    std::string current_header_field_;
    std::string current_header_value_;

    http_parser parser_;

private:
    static http_parser_settings settings_;

    DISABLE_COPY_AND_ASSIGNMENT(HttpParser);
};

#endif // HTTP_PARSER_HPP
