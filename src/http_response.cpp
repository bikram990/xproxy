#include "message/http/http_response.hpp"

namespace xproxy {
namespace message {
namespace http {

std::string HttpResponse::getField(HttpMessage::FieldType type) const {
    switch (type) {
    case FieldType::kResponseStatus:
        return std::to_string(status_);
    case FieldType::kResponseMessage:
        return message_;
    case FieldType::kRequestMethod:
    case FieldType::kRequestUri:
    default:
        ; // ignore
    }
    return std::string();
}

void HttpResponse::setField(HttpMessage::FieldType type, std::string&& value) {
    switch (type) {
    case FieldType::kResponseStatus:
        status_ = std::stoi(value);
        break;
    case FieldType::kResponseMessage:
        message_ = std::move(value);
        break;
    case FieldType::kRequestMethod:
    case FieldType::kRequestUri:
    default:
        ; // ignore
    }
}

void HttpResponse::reset() {
    HttpMessage::reset();
    status_ = 0;
    message_.clear();
}

std::size_t HttpResponse::serializeFirstLine(memory::ByteBuffer &buffer) {
    auto orig_size = buffer.size();
    buffer << "HTTP/" << major_version_ << '.' << minor_version_ << ' '
           << status_ << ' ' << message_ << CRLF;
    return buffer.size() - orig_size;
}

} // namespace http
} // namespace message
} // namespace xproxy
