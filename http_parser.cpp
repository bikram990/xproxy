#include "http_parser.h"

http_parser_settings HttpParser::settings_ = {
    &HttpParser::OnMessageBegin,
    &HttpParser::OnUrl,
    &HttpParser::OnStatusComplete,
    &HttpParser::OnHeaderField,
    &HttpParser::OnHeaderValue,
    &HttpParser::OnHeadersComplete,
    &HttpParser::OnBody,
    &HttpParser::OnMessageComplete
};

HttpParser::HttpParser(http_parser_type type) {
    http_parser_init(&parser_, type);
    parser_.data = this;
}

HttpParser::~HttpParser() {}

int HttpParser::OnMessageBegin(http_parser *parser) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnMessageBegin();
}

int HttpParser::OnUrl(http_parser *parser, const char *at, std::size_t length) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnUrl(at, length);
}

int HttpParser::OnStatusComplete(http_parser *parser) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnStatusComplete();
}

int HttpParser::OnHeaderField(http_parser *parser, const char *at, std::size_t length) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnHeaderField(at, length);
}

int HttpParser::OnHeaderValue(http_parser *parser, const char *at, std::size_t length) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnHeaderValue(at, length);
}

int HttpParser::OnHeadersComplete(http_parser *parser) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnHeadersComplete();
}

int HttpParser::OnBody(http_parser *parser, const char *at, std::size_t length) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnBody(at, length);
}

int HttpParser::OnMessageComplete(http_parser *parser) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnMessageComplete();
}
