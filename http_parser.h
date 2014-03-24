#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "common.h"
#include "libs/http-parser/http_parser.h"

class HttpParser {
protected:
    HttpParser(http_parser_type type);
    virtual ~HttpParser();

    virtual int OnMessageBegin() = 0;
    virtual int OnUrl(const char *at, std::size_t length) = 0;
    virtual int OnStatus() = 0;
    virtual int OnHeaderField(const char *at, std::size_t length) = 0;
    virtual int OnHeaderValue(const char *at, std::size_t length) = 0;
    virtual int OnHeadersComplete() = 0;
    virtual int OnBody(const char *at, std::size_t length) = 0;
    virtual int OnMessageComplete() = 0;

private:
    static int OnMessageBegin(http_parser *parser);
    static int OnUrl(http_parser *parser, const char *at, std::size_t length);
    static int OnStatus(http_parser *parser);
    static int OnHeaderField(http_parser *parser, const char *at, std::size_t length);
    static int OnHeaderValue(http_parser *parser, const char *at, std::size_t length);
    static int OnHeadersComplete(http_parser *parser);
    static int OnBody(http_parser *parser, const char *at, std::size_t length);
    static int OnMessageComplete(http_parser *parser);

private:
    DISABLE_COPY_AND_ASSIGNMENT(HttpParser);

protected:
    http_parser parser_;

private:
    static http_parser_settings settings_;
};

#endif // HTTP_PARSER_H
