#include "http_parser.h"
#include "log.h"

http_parser_settings HttpParser::settings_ = {
    &HttpParser::MessageBegin,
    &HttpParser::Url,
    &HttpParser::Status,
    &HttpParser::HeaderField,
    &HttpParser::HeaderValue,
    &HttpParser::HeadersComplete,
    &HttpParser::Body,
    &HttpParser::MessageComplete
};

HttpParser::HttpParser(http_parser_type type) {
    http_parser_init(&parser_, type);
    parser_.data = this;
}

HttpParser::~HttpParser() {}

bool HttpParser::consume(boost::asio::streambuf& buffer) {
    std::size_t orig_size = buffer.size();
    std::size_t consumed = http_parser_execute(&parser_, &settings_,
                                               boost::asio::buffer_cast<const char *>(buffer.data()), buffer.size());
    buffer.consume(consumed);
    if (consumed != orig_size) {
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

int HttpParser::MessageBegin(http_parser *parser) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnMessageBegin();
}

int HttpParser::Url(http_parser *parser, const char *at, std::size_t length) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnUrl(at, length);
}

int HttpParser::Status(http_parser *parser, const char *at, std::size_t length) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnStatus(at, length);
}

int HttpParser::HeaderField(http_parser *parser, const char *at, std::size_t length) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnHeaderField(at, length);
}

int HttpParser::HeaderValue(http_parser *parser, const char *at, std::size_t length) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnHeaderValue(at, length);
}

int HttpParser::HeadersComplete(http_parser *parser) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnHeadersComplete();
}

int HttpParser::Body(http_parser *parser, const char *at, std::size_t length) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnBody(at, length);
}

int HttpParser::MessageComplete(http_parser *parser) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);
    return p->OnMessageComplete();
}
