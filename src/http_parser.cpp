#include "http_message.hpp"
#include "http_parser.hpp"
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

HttpParser::HttpParser(std::shared_ptr<Connection> connection,
                       http_parser_type type)
    : connection_(connection),
      header_completed_(false),
      message_completed_(false),
      chunked_(false) {
    ::http_parser_init(&parser_, type);
    parser_.data = this;

    InitMessage();
}

bool HttpParser::KeepAlive() const {
    if (!header_completed_) // return false when header is incomplete
        return false;
    return ::http_should_keep_alive(const_cast<http_parser*>(&parser_)) != 0;
}

void HttpParser::reset() {
    ::http_parser_init(&parser_, static_cast<http_parser_type>(parser_.type));
    header_completed_ = false;
    message_completed_ = false;
    chunked_ = false;
    current_header_field_.clear();
    current_header_value_.clear();
    message_->reset();
}

int HttpParser::OnMessageBegin(http_parser *parser) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);

    p->header_completed_ = false;
    p->message_completed_ = false;
    return 0;
}

int HttpParser::OnUrl(http_parser *parser, const char *at, std::size_t length) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);

    auto method = ::http_method_str(static_cast<http_method>(parser->method));
    p->message_->SetField(HttpMessage::kRequestMethod,
                          std::move(std::string(method)));
    p->message_->SetField(HttpMessage::kRequestUri,
                          std::move(std::string(at, length)));
    return 0;
}

int HttpParser::OnStatus(http_parser *parser, const char *at, std::size_t length) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);

    p->message_->SetField(HttpMessage::kResponseStatus,
                          std::move(std::to_string(parser_.status_code)));
    p->message_->SetField(HttpMessage::kResponseMessage,
                          std::move(std::string(at, length)));
    return 0;
}

int HttpParser::OnHeaderField(http_parser *parser, const char *at, std::size_t length) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);

    if (!p->current_header_value_.empty()) {
        p->message_->AddHeader(p->current_header_field_,
                               p->current_header_value_);
        p->current_header_field_.clear();
        p->current_header_value_.clear();
    }
    p->current_header_field_.append(at, length);
    return 0;
}

int HttpParser::OnHeaderValue(http_parser *parser, const char *at, std::size_t length) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);

    p->current_header_value_.append(at, length);
    return 0;
}

int HttpParser::OnHeadersComplete(http_parser *parser) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);

    if (!p->current_header_value_.empty()) {
        p->message_->AddHeader(p->current_header_field_,
                               p->current_header_value_);
        p->current_header_field_.clear();
        p->current_header_value_.clear();
    }
    p->header_completed_ = true;

    if (p->parser_.flags & F_CHUNKED)
        p->chunked_ = true;

    // TODO we need do more thing here, e.g. set the keep_alive flag

    return 0;
}

int HttpParser::OnBody(http_parser *parser, const char *at, std::size_t length) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);

    if (p->chunked_) {
        std::ostringstream out;
        out << std::hex << length;
        out.copyfmt(std::ios(nullptr));
        out << CRLF;
        p->message_->AppendBody(out.str());
    }
    p->message_->AppendBody(at, length, false);
    if (p->chunked_) {
        p->message_->AppendBody(CRLF, 2, false);
    }

    return 0;
}

int HttpParser::OnMessageComplete(http_parser *parser) {
    HttpParser *p = static_cast<HttpParser*>(parser->data);
    assert(p);

    if (p->chunked_) {
        p->message_->AppendBody(END_CHUNK, 5, false);
    }

    p->message_completed_ = true;

    return 0;
}

void HttpParser::InitMessage(http_parser_type type) {
    switch (type) {
    case HTTP_REQUEST:
        message_.reset(new HttpRequest);
        break;
    case HTTP_RESPONSE:
        message_.reset(new HttpResponse);
        break;
    case HTTP_BOTH:
    default:
        ; // TODO ignore here?
    }
}
