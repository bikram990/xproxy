#include "memory/byte_buffer.hpp"
#include "message/http/http_message.hpp"

namespace xproxy {
namespace message {
namespace http {

HttpMessage::HttpMessage()
    : major_version_(1),
      minor_version_(1),
      helper_(*this) {}

HttpMessage::SerializeHelper::SerializeHelper(HttpMessage &message)
    : message_(message),
      headers_serialized_(false),
      body_serialized_(0) {}

std::size_t HttpMessage::serialize(memory::ByteBuffer &buffer) {
    return helper_.serialize(buffer);
}

bool HttpMessage::findHeader(const std::string &name, std::string &value) const {
    auto it = headers_.find(name);
    if (it == headers_.end())
        return false;
    value = it->second;
    return true;
}

HttpMessage &HttpMessage::addHeader(const std::string &name, const std::string &value) {
    headers_[name] = value;
    return *this;
}

HttpMessage &HttpMessage::appendBody(const char *data, std::size_t size) {
    body_ << ByteBuffer::wrap(data, size);
    return *this;
}

void HttpMessage::reset() {
    major_version_ = 1;
    minor_version_ = 1;
    headers_.clear();
    body_.clear();
    helper_.reset();
}

std::size_t HttpMessage::serializeHeaders(memory::ByteBuffer &buffer) {
    auto orig_size = buffer.size();
    for (auto it : headers_) {
        buffer << it.first << ": " << it.second << CRLF;
    }
    buffer << CRLF;
    return buffer.size() - orig_size;
}

std::size_t HttpMessage::SerializeHelper::serialize(memory::ByteBuffer &buffer) {
    std::size_t size = 0;
    if (!headers_serialized_) {
        assert(body_serialized_ == 0);

        size += message_.serializeFirstLine(buffer);
        size += message_.serializeHeaders(buffer);
        headers_serialized_ = true;
    }

    assert(message_.body_.size() >= body_serialized_);

    if (message_.body_.size() == body_serialized_)
        return size;

    auto inc = message_.body_.size() - body_serialized_;
    buffer << memory::ByteBuffer::wrap(message_.body_.data(body_serialized_), inc);
    body_serialized_ += inc;
    size += inc;

    return size;
}

void HttpMessage::SerializeHelper::reset() {
    headers_serialized_ = false;
    body_serialized_ = 0;
}

} // namespace http
} // namespace message
} // namespace xproxy
