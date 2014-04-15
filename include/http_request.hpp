#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include "http_message.hpp"

class HttpRequest : public HttpMessage {
public:
    HttpRequest() = default;
    virtual ~HttpRequest() = default;

    virtual std::string GetField(FieldType type) const {
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
        return std::string{};
    }

    virtual void SetField(FieldType type, std::string&& value) {
        switch (type) {
        case FieldType::kRequestMethod:
            method_ = std::move(value);
            if (buf_sync_)
                UpdateFirstLine();
            break;
        case FieldType::kRequestUri:
            uri_ = std::move(value);
            if (buf_sync_)
                UpdateFirstLine();
            break;
        case FieldType::kResponseStatus:
        case FieldType::kResponseMessage:
        default:
            ; // ignore
        }
    }

    virtual void reset() {
        HttpMessage::reset();
        method_.clear();
        uri_.clear();
    }

protected:
    virtual void UpdateFirstLine() {
        ByteBuffer temp(method_.length() + uri_.length() + 100);
        temp << method_ << ' ' << uri_  << ' ' << "HTTP/"
             << major_version_ << '.' << minor_version_ << CRLF;
        UpdateSegment(0, temp);
    }

private:
    std::string method_;
    std::string uri_;
};

#endif // HTTP_REQUEST_HPP
