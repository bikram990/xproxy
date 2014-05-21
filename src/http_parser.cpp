#include "xproxy/log/log.hpp"
#include "xproxy/message/http/http_message.hpp"
#include "xproxy/message/http/http_parser.hpp"

namespace xproxy {
namespace message {
namespace http {

http_parser_settings HttpParser::settings_ = {
    &HttpParser::onMessageBegin,
    &HttpParser::onUrl,
    &HttpParser::onStatus,
    &HttpParser::onHeaderField,
    &HttpParser::onHeaderValue,
    &HttpParser::onHeadersComplete,
    &HttpParser::onBody,
    &HttpParser::onMessageComplete
};

std::size_t HttpParser::consume(const char* at, std::size_t length) {
    auto consumed = ::http_parser_execute(&parser_, &settings_, at, length);

    if (consumed != length) {
        if (HTTP_PARSER_ERRNO(&parser_) != HPE_OK) {
            XERROR << "Message parsing, error code: " << parser_.http_errno
                   << ", message: " << errorMessage();
            return 0;
        } else {
            // TODO will this happen? a message is parsed, but there is still data?
            XWARN << "Weird: parser does not consume all data, but there is no error.";
            return consumed;
        }
    }

    return consumed;
}

bool HttpParser::keepAlive() const {
    if (!headers_completed_) // return false when headers are incomplete
        return false;
    return ::http_should_keep_alive(const_cast<http_parser*>(&parser_)) != 0;
}

void HttpParser::reset() {
    ::http_parser_init(&parser_, static_cast<http_parser_type>(parser_.type));
    headers_completed_ = false;
    message_completed_ = false;
    chunked_ = false;
    current_header_field_.clear();
    current_header_value_.clear();
}

int HttpParser::onMessageBegin(http_parser *parser) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    p->headers_completed_ = false;
    p->message_completed_ = false;
    p->chunked_ = false;
    p->current_header_field_.clear();
    p->current_header_value_.clear();
    return 0;
}

int HttpParser::onUrl(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    auto method = ::http_method_str(static_cast<http_method>(parser->method));
    p->message_.setField(HttpMessage::kRequestMethod,
                         std::move(std::string(method)));
    p->message_.setField(HttpMessage::kRequestUri,
                         std::move(std::string(at, length)));
    return 0;
}

int HttpParser::onStatus(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    p->message_.setField(HttpMessage::kResponseStatus,
                         std::move(std::to_string(p->parser_.status_code)));
    p->message_.setField(HttpMessage::kResponseMessage,
                         std::move(std::string(at, length)));
    return 0;
}

int HttpParser::onHeaderField(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    if (!p->current_header_value_.empty()) {
        p->message_.addHeader(p->current_header_field_, p->current_header_value_);
        p->current_header_field_.clear();
        p->current_header_value_.clear();
    }
    p->current_header_field_.append(at, length);
    return 0;
}

int HttpParser::onHeaderValue(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    p->current_header_value_.append(at, length);
    return 0;
}

int HttpParser::onHeadersComplete(http_parser *parser) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    // current_header_value_ MUST NOT be empty
    assert(!p->current_header_value_.empty());

    p->message_.addHeader(p->current_header_field_, p->current_header_value_);
    p->current_header_field_.clear();
    p->current_header_value_.clear();

    p->headers_completed_ = true;

    if (p->parser_.flags & F_CHUNKED)
        p->chunked_ = true;

    if (p->observer_)
        p->observer_->onHeadersComplete(p->message_);

    return 0;
}

int HttpParser::onBody(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    if (p->chunked_) {
        std::ostringstream out;
        out << std::hex << length;
        out.copyfmt(std::ios(nullptr));
        out << CRLF;
        p->message_.appendBody(out.str());
    }
    p->message_.appendBody(at, length);
    if (p->chunked_) {
        p->message_.appendBody(CRLF, 2);
    }

    if (p->observer_)
        p->observer_->onBody(p->message_);

    return 0;
}

int HttpParser::onMessageComplete(http_parser *parser) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    if (p->chunked_) {
        p->message_.appendBody(END_CHUNK, 5);
    }

    p->message_completed_ = true;

    if (p->observer_)
        p->observer_->onMessageComplete(p->message_);

    return 0;
}

} // namespace http
} // namespace message
} // namespace xproxy
