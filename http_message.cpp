#include "http_message.h"
#include "log.h"

http_parser_settings HttpMessage::settings_ = {
    &HttpMessage::MessageBeginCallback,
    &HttpMessage::UrlCallback,
    &HttpMessage::StatusCallback,
    &HttpMessage::HeaderFieldCallback,
    &HttpMessage::HeaderValueCallback,
    &HttpMessage::HeadersCompleteCallback,
    &HttpMessage::BodyCallback,
    &HttpMessage::MessageCompleteCallback
};

HttpMessage::HttpMessage(http_parser_type type)
    : header_completed_(false),
      message_completed_(false) {
    ::http_parser_init(&parser_, type);
    parser_.data = this;
}

bool HttpMessage::consume(boost::asio::streambuf& buffer) {
    std::size_t orig_size = buffer.size();
    std::size_t consumed = ::http_parser_execute(&parser_, &settings_,
                                                 boost::asio::buffer_cast<const char *>(buffer.data()),
                                                 buffer.size());
    buffer.consume(consumed);
    if (consumed != orig_size) {
        if (HTTP_PARSER_ERRNO(&parser_) != HPE_OK) {
            XERROR << "Error occurred during message parsing, error code: "
                   << parser_.http_errno << ", message: "
                   << ::http_errno_description(static_cast<http_errno>(parser_.http_errno));
            return false;
        } else {
            // TODO will this happen? a message is parsed, but there is still data?
            XWARN << "Weird: parser does not consume all data, but there is no error.";
            return true;
        }
    }

    return true;
}

void HttpMessage::reset() {
    ::http_parser_init(&parser_, static_cast<http_parser_type>(parser_.type));
    header_completed_ = false;
    message_completed_ = false;
    current_header_field_.clear();
    current_header_value_.clear();
    headers_.reset();
    if (body_.size())
        body_.consume(body_.size());
}

int HttpMessage::MessageBeginCallback(http_parser *parser) {
    HttpMessage *m = static_cast<HttpMessage*>(parser->data);
    assert(m);

    m->header_completed_ = false;
    m->message_completed_ = false;
    return 0;
}

int HttpMessage::UrlCallback(http_parser *parser, const char *at, std::size_t length) {
    HttpMessage *m = static_cast<HttpMessage*>(parser->data);
    assert(m);
    return m->OnUrl(at, length);
}

int HttpMessage::StatusCallback(http_parser *parser, const char *at, std::size_t length) {
    HttpMessage *m = static_cast<HttpMessage*>(parser->data);
    assert(m);
    return m->OnStatus(at, length);
}

int HttpMessage::HeaderFieldCallback(http_parser *parser, const char *at, std::size_t length) {
    HttpMessage *m = static_cast<HttpMessage*>(parser->data);
    assert(m);

    if (!m->current_header_value_.empty()) {
        m->headers_.add(HttpHeader(m->current_header_field_, m->current_header_value_));
        m->current_header_field_.clear();
        m->current_header_value_.clear();
    }
    m->current_header_field_.append(at, length);
    return 0;
}

int HttpMessage::HeaderValueCallback(http_parser *parser, const char *at, std::size_t length) {
    HttpMessage *m = static_cast<HttpMessage*>(parser->data);
    assert(m);

    m->current_header_value_.append(at, length);
    return 0;
}

int HttpMessage::HeadersCompleteCallback(http_parser *parser) {
    HttpMessage *m = static_cast<HttpMessage*>(parser->data);
    assert(m);

    if (!m->current_header_value_.empty()) {
        m->headers_.add(HttpHeader(m->current_header_field_, m->current_header_value_));
        m->current_header_field_.clear();
        m->current_header_value_.clear();
    }
    m->header_completed_ = true;
    return 0;
}

int HttpMessage::BodyCallback(http_parser *parser, const char *at, std::size_t length) {
    HttpMessage *m = static_cast<HttpMessage*>(parser->data);
    assert(m);

    boost::asio::buffer_copy(m->body_.prepare(length), boost::asio::buffer(at, length));
    m->body_.commit(length);
    return 0;
}

int HttpMessage::MessageCompleteCallback(http_parser *parser) {
    HttpMessage *m = static_cast<HttpMessage*>(parser->data);
    assert(m);

    m->message_completed_ = true;
    return 0;
}
