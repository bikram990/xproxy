#ifndef HTTP_MESSAGE_HPP
#define HTTP_MESSAGE_HPP

#include "common.h"
#include "memory/segmental_byte_buffer.hpp"

class HttpMessage {
private:
    class SerializeHelper {
    public:
        SerializeHelper(HttpMessage& message)
            : message_(message),
              headers_serialized_(false),
              body_serialized_(0) {}

        ~SerializeHelper() = default;

        std::size_t serialize(ByteBuffer& buffer) {
            std::size_t size = 0;
            if (!headers_serialized_) {
                assert(body_serialized_ == 0);

                size += message_.SerializeFirstLine(buffer);
                size += message_.SerializeHeaders(buffer);
                headers_serialized_ = true;
            }

            assert(message_.body_.size() >= body_serialized_);

            if (message_.body_.size() == body_serialized_)
                return size;

            buffer << ByteBuffer.wrap(message_.body_.data() + body_serialized_,
                                      message_.body_.size() - body_serialized_);
            body_serialized_ = message_.body_.size();

            return size + message_.body_.size() - body_serialized_;
        }

        void reset() {
            headers_serialized_ = false;
            body_serialized_ = 0;
        }

    private:
        HttpMessage& message_;
        bool headers_serialized_;
        std::size_t body_serialized_;
    };

public:
    enum FieldType {
        kRequestMethod, kRequestUri,
        kResponseStatus, kResponseMessage
    };

public:
    int MajorVersion() const { return major_version_; }
    int MinorVersion() const { return minor_version_; }
    void MajorVersion(int version) { major_version_ = version; }
    void MinorVersion(int version) { minor_version_ = version; }

    bool FindHeader(const std::string& name, std::string& value) const {
        auto it = headers_.find(name);
        if (it == headers_.end())
            return false;
        value = it->second;
        return true;
    }

    HttpMessage& AddHeader(const std::string& name, const std::string& value) {
        headers_[name] = value;
        return *this;
    }

    HttpMessage& AppendBody(const char *data, std::size_t size) {
        body_ << ByteBuffer.wrap(data, size);
        return *this;
    }

    std::size_t serialize(ByteBuffer& buffer) {
        return helper_.serialize(buffer);
    }

public:
    virtual std::string GetField(FieldType type) const = 0;

    virtual void SetField(FieldType type, std::string&& value) = 0;

    virtual void reset() {
        major_version_ = 1;
        minor_version_ = 1;
        headers_.clear();
        body_.clear();
        helper_.reset();
    }

protected:
    HttpMessage() : major_version_(1), minor_version_(1), helper_(*this) {}
    virtual ~HttpMessage() = default;

    virtual std::size_t SerializeFirstLine(ByteBuffer& buffer) = 0;

    std::size_t SerializeHeaders(ByteBuffer& buffer) {
        auto orig_size = buffer.size();
        for (auto it : headers_) {
            buffer << it.first << ": " << it.second << CRLF;
        }
        buffer << CRLF;
        return buffer.size() - orig_size;
    }

protected:
    int major_version_;
    int minor_version_;

private:
    std::map<std::string, std::string> headers_;
    ByteBuffer body_;
    SerializeHelper helper_;

private:
    DISABLE_COPY_AND_ASSIGNMENT(HttpMessage);
};

#endif // HTTP_MESSAGE_HPP
