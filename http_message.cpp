#include "connection.h"
#include "http_message.h"
#include "log.h"

#define CRLF "\r\n"
#define END_CHUNK "0\r\n\r\n"

typedef void(Connection::*callback_ptr)();
std::vector<callback_ptr> HttpMessage::callbacks_ = {
    &Connection::OnHeadersComplete,
    &Connection::OnBody,
    &Connection::OnBodyComplete
};

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

HttpMessage::HttpMessage(std::shared_ptr<Connection> connection, http_parser_type type)
    : header_completed_(false),
      message_completed_(false),
      connection_(connection),
      chunked_(false),
      callback_choice_(-1) {
    ::http_parser_init(&parser_, type);
    parser_.data = this;
}

bool HttpMessage::consume(boost::asio::streambuf& buffer) {
    std::size_t orig_size = buffer.size();
    std::size_t consumed = ::http_parser_execute(&parser_, &settings_,
                                                 boost::asio::buffer_cast<const char *>(buffer.data()),
                                                 buffer.size());
    buffer.consume(consumed);

    if (callback_choice_ >= 0 && callback_choice_ < callbacks_.size()) {
        std::shared_ptr<Connection> c(connection_.lock());
        if (c)
            c->service().post(std::bind(callbacks_[callback_choice_], c));
    }

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

    if (m->parser_.flags & F_CHUNKED)
        m->chunked_ = true;

    XDEBUG << "header complete";
    m->callback_choice_ = kHeadersComplete;

    return 0;
}

int HttpMessage::BodyCallback(http_parser *parser, const char *at, std::size_t length) {
    HttpMessage *m = static_cast<HttpMessage*>(parser->data);
    assert(m);

    if (m->chunked_) {
        std::ostream out(&m->body_);
        out << std::hex << length;
        out.copyfmt(std::ios(nullptr));
        out << CRLF;
    }
    boost::asio::buffer_copy(m->body_.prepare(length), boost::asio::buffer(at, length));
    m->body_.commit(length);
    if (m->chunked_) {
        std::ostream out(&m->body_);
        out << CRLF;
    }

    XDEBUG << "body";
    m->callback_choice_ = kBody;

    return 0;
}

int HttpMessage::MessageCompleteCallback(http_parser *parser) {
    HttpMessage *m = static_cast<HttpMessage*>(parser->data);
    assert(m);

    if (m->chunked_) {
        std::ostream out(&m->body_);
        out << END_CHUNK;
    }

    m->message_completed_ = true;

    XDEBUG << "message complete";
    m->callback_choice_ = kBodyComplete;

    return 0;
}
