#include "connection_adapter.hpp"
#include "http_message.hpp"
#include "http_parser.hpp"

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

std::size_t HttpParser::consume(const char* at, std::size_t length) {
    auto consumed = ::http_parser_execute(&parser_, &settings_, at, length);

    if (consumed != length) {
        if (HTTP_PARSER_ERRNO(&parser_) != HPE_OK) {
            XERROR << "Error occurred during message parsing, error code: "
                   << parser_.http_errno << ", message: "
                   << ::http_errno_description(static_cast<http_errno>(parser_.http_errno));
            return 0;
        } else {
            // TODO will this happen? a message is parsed, but there is still data?
            XWARN << "Weird: parser does not consume all data, but there is no error.";
            return consumed;
        }
    }

    return consumed;
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
    // message_.reset(); // TODO should we reset the message here?
}

int HttpParser::OnMessageBegin(http_parser *parser) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    p->header_completed_ = false;
    p->message_completed_ = false;
    p->chunked_ = false;
    p->current_header_field_.clear();
    p->current_header_value_.clear();
    return 0;
}

int HttpParser::OnUrl(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    auto method = ::http_method_str(static_cast<http_method>(parser->method));
    p->message_.SetField(HttpMessage::kRequestMethod,
                         std::move(std::string(method)));
    p->message_.SetField(HttpMessage::kRequestUri,
                         std::move(std::string(at, length)));
    return 0;
}

int HttpParser::OnStatus(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    p->message_.SetField(HttpMessage::kResponseStatus,
                         std::move(std::to_string(p->parser_.status_code)));
    p->message_.SetField(HttpMessage::kResponseMessage,
                         std::move(std::string(at, length)));
    return 0;
}

int HttpParser::OnHeaderField(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    if (!p->current_header_value_.empty()) {
        p->message_.AddHeader(p->current_header_field_,
                              p->current_header_value_);
        p->current_header_field_.clear();
        p->current_header_value_.clear();
    }
    p->current_header_field_.append(at, length);
    return 0;
}

int HttpParser::OnHeaderValue(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    p->current_header_value_.append(at, length);
    return 0;
}

int HttpParser::OnHeadersComplete(http_parser *parser) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    // current_header_value_ MUST NOT be empty
    assert(!p->current_header_value_.empty());

    p->message_->AddHeader(p->current_header_field_,
                           p->current_header_value_);
    p->current_header_field_.clear();
    p->current_header_value_.clear();

    p->header_completed_ = true;

    if (p->parser_.flags & F_CHUNKED)
        p->chunked_ = true;

    if (adapter_)
        adapter_->OnHeadersComplete();

    return 0;
}

int HttpParser::OnBody(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    if (p->chunked_) {
        std::ostringstream out;
        out << std::hex << length;
        out.copyfmt(std::ios(nullptr));
        out << CRLF;
        p->message_.AppendBody(out.str());
    }
    p->message_.AppendBody(at, length);
    if (p->chunked_) {
        p->message_.AppendBody(CRLF, 2);
    }

    if (adapter_)
        adapter_->OnBody();

    return 0;
}

int HttpParser::OnMessageComplete(http_parser *parser) {
    auto p = static_cast<HttpParser*>(parser->data);
    assert(p);

    if (p->chunked_) {
        p->message_.AppendBody(END_CHUNK, 5);
    }

    p->message_completed_ = true;

    if (adapter_)
        adapter_->OnBodyComplete();

    return 0;
}
