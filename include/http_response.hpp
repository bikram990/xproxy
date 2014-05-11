#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "http_message.hpp"

class HttpResponse : public HttpMessage {
public:
    HttpResponse() = default;
    virtual ~HttpResponse() = default;

    virtual std::string GetField(FieldType type) const {
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

    virtual void SetField(FieldType type, std::string&& value) {
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

    virtual void reset() {
        HttpMessage::reset();
        status_ = 0;
        message_.clear();
    }

protected:
    virtual std::size_t SerializeFirstLine(ByteBuffer& buffer) {
        auto orig_size = buffer.size();
        buffer << "HTTP/" << major_version_ << '.' << minor_version_ << ' '
               << status_ << ' ' << message_ << CRLF;
        return buffer.size() - orig_size;
    }

private:
    int status_;
    std::string message_;
};

#endif // HTTP_RESPONSE_HPP
