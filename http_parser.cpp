#include "http_parser.h"
#include "log.h"

http_parser_settings HttpParser::settings_ = {
    &HttpParser::OnMessageBegin,
    &HttpParser::OnUrl,
    &HttpParser::OnStatus,
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

bool HttpParser::consume(boost::asio::streambuf& buffer) {
    std::size_t consumed = http_parser_execute(&parser_, &settings_, buffer.data(), buffer.size());
    if (consumed != buffer.size()) {
        if (HTTP_PARSER_ERRNO(&parser_) != HPE_OK) {
            XERROR << "Error occurred during message parsing, error code: "
                   << parser_.http_errno << ", message: "
                   << http_errno_description(static_cast<http_errno>(parser_.http_errno));
            return false;
        } else {
            // TODO will this happen? a message is parsed, but there is still data?
            XWARN << "Weird: parser does not consume all data, but there is no error.";
            return true;
        }
    }

    return true;
}

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

int HttpParser::OnStatus(http_parser *parser) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnStatus();
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
