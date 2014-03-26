#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "common.h"
#include "libs/http-parser/http_parser.h"

class HttpParser {
public:
    bool consume(boost::asio::streambuf& buffer);

public:
    static int MessageBegin(http_parser *parser);
    static int Url(http_parser *parser, const char *at, std::size_t length);
    static int Status(http_parser *parser, const char *at, std::size_t length);
    static int HeaderField(http_parser *parser, const char *at, std::size_t length);
    static int HeaderValue(http_parser *parser, const char *at, std::size_t length);
    static int HeadersComplete(http_parser *parser);
    static int Body(http_parser *parser, const char *at, std::size_t length);
    static int MessageComplete(http_parser *parser);

protected:
    HttpParser(http_parser_type type);
    virtual ~HttpParser();

    virtual int OnMessageBegin() = 0;
    virtual int OnUrl(const char *at, std::size_t length) = 0;
    virtual int OnStatus(const char *at, std::size_t length) = 0;
    virtual int OnHeaderField(const char *at, std::size_t length) = 0;
    virtual int OnHeaderValue(const char *at, std::size_t length) = 0;
    virtual int OnHeadersComplete() = 0;
    virtual int OnBody(const char *at, std::size_t length) = 0;
    virtual int OnMessageComplete() = 0;

private:
    DISABLE_COPY_AND_ASSIGNMENT(HttpParser);

protected:
    http_parser parser_;

private:
    static http_parser_settings settings_;
};

#endif // HTTP_PARSER_H
