#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "http_message.hpp"

class HttpResponse : public HttpMessage {
public:
    HttpResponse() = default;
    virtual ~HttpResponse() = default;

    virtual std::string GetField(FieldType type) {
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
        return std::string{};
    }

    virtual void SetField(FieldType type, std::string&& value) {
        switch (type) {
        case FieldType::kResponseStatus:
            status_ = std::stoi(value);
            if (buf_sync_)
                UpdateFirstLine();
            break;
        case FieldType::kResponseMessage:
            message_ = std::move(value);
            if (buf_sync_)
                UpdateFirstLine();
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
    virtual void UpdateFirstLine() {
        ByteBuffer temp(message_.length() + 100);
        temp << "HTTP/" << major_version_ << '.' << minor_version_ << ' '
             << status_ << ' ' << message_ << CRLF;
        UpdateSegment(0, temp);
    }

private:
    int status_;
    std::string message_;
};

#endif // HTTP_RESPONSE_HPP
