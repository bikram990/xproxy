#include "http_message.h"

HttpMessage::HttpMessage(http_parser_type type)
    : completed_(false),
      HttpParser(type) {}

HttpMessage::~HttpMessage() {}

void HttpMessage::reset() {
    completed_ = false;
    current_header_field_.clear();
    current_header_value_.clear();
    headers_.reset();
    if (body_.size())
        body_.consume(body_.size());
}

int HttpMessage::OnMessageBegin() {
    completed_ = false;
    return 0;
}

int HttpMessage::OnUrl(const char *at, std::size_t length) {
    return 0;
}

int HttpMessage::OnStatus() {
    return 0;
}

int HttpMessage::OnHeaderField(const char *at, std::size_t length) {
    if (!current_header_value_.empty()) {
        headers_.PushBack(HttpHeader(current_header_field_, current_header_value_));
        current_header_field_.clear();
        current_header_value_.clear();
    }

    current_header_field_.append(at, length);

    return 0;
}

int HttpMessage::OnHeaderValue(const char *at, std::size_t length) {
    current_header_value_.append(at, length);
    return 0;
}

int HttpMessage::OnHeadersComplete() {
    if (!current_header_value_.empty()) {
        headers_.PushBack(HttpHeader(current_header_field_, current_header_value_));
        current_header_field_.clear();
        current_header_value_.clear();
    }

    return 0;
}

int HttpMessage::OnBody(const char *at, std::size_t length) {
    boost::asio::buffer_copy(body_.prepare(length),
                             boost::asio::buffer(at, length));
    body_.commit(length);
    return 0;
}

int HttpMessage::OnMessageComplete() {
    completed_ = true;
    return 0;
}
