#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "common.h"
#include "http_message.h"
#include "libs/http-parser/http_parser.h"

class HttpParser {
public:
    static int OnMessageBegin(http_parser *parser);
    static int OnUrl(http_parser *parser, const char *at, std::size_t length);
    static int OnStatusComplete(http_parser *parser);
    static int OnHeaderField(http_parser *parser, const char *at, std::size_t length);
    static int OnHeaderValue(http_parser *parser, const char *at, std::size_t length);
    static int OnHeadersComplete(http_parser *parser);
    static int OnBody(http_parser *parser, const char *at, std::size_t length);
    static int OnMessageComplete(http_parser *parser);

public:
    HttpParser(http_parser_type type);
    virtual ~HttpParser();

    virtual int OnMessageBegin() { return 0; }
    virtual int OnUrl(const char *at, std::size_t length) { return 0; }
    virtual int OnStatusComplete() { return 0; }
    virtual int OnHeaderField(const char *at, std::size_t length) { return 0; }
    virtual int OnHeaderValue(const char *at, std::size_t length) { return 0; }
    virtual int OnHeadersComplete() { return 0; }
    virtual int OnBody(const char *at, std::size_t length) { return 0; }
    virtual int OnMessageComplete() { return 0; }

    Message *message() const { return message_.get(); }

private:
    DISABLE_COPY_AND_ASSIGNMENT(HttpParser);

protected:
    http_parser parser_;
    std::unique_ptr<HttpMessage> message_; // TODO should we use shared_ptr here?

private:
    static http_parser_settings settings_;
};

#endif // HTTP_PARSER_H
