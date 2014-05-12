#include "message/http/http_request.hpp"

namespace xproxy {
namespace message {
namespace http {

std::string HttpRequest::getField(HttpMessage::FieldType type) const {
    switch (type) {
    case FieldType::kRequestMethod:
        return method_;
    case FieldType::kRequestUri:
        return uri_;
    case FieldType::kResponseStatus:
    case FieldType::kResponseMessage:
    default:
        ; // ignore
    }
    return std::string();
}

void HttpRequest::setField(HttpMessage::FieldType type, std::string &value) {
    switch (type) {
    case FieldType::kRequestMethod:
        method_ = std::move(value);
        break;
    case FieldType::kRequestUri:
        uri_ = std::move(value);
        break;
    case FieldType::kResponseStatus:
    case FieldType::kResponseMessage:
    default:
        ; // ignore
    }
}

void HttpRequest::reset() {
    HttpMessage::reset();
    method_.clear();
    uri_.clear();
}

std::size_t HttpRequest::serializeFirstLine(memory::ByteBuffer &buffer) {
    auto orig_size = buffer.size();
    buffer << method_ << ' ' << uri_  << ' ' << "HTTP/"
           << major_version_ << '.' << minor_version_ << CRLF;
    return buffer.size() - orig_size;
}

} // namespace http
} // namespace message
} // namespace xproxy
