#ifndef HTTP_MESSAGE_HPP
#define HTTP_MESSAGE_HPP

#include <string>
#include <map>
#include "common.hpp"
#include "message/message.hpp"

namespace xproxy {
namespace message {
namespace http {

/**
 * @brief The HttpMessage class
 *
 * This class represents a HTTP message, either request or response.
 */
class HttpMessage : public Message {
public:
    enum FieldType {
        kRequestMethod, kRequestUri,
        kResponseStatus, kResponseMessage
    };

    int getMajorVersion() const { return major_version_; }
    int getMinorVersion() const { return minor_version_; }
    void setMajorVersion(int version) { major_version_ = version; }
    void setMinorVersion(int version) { minor_version_ = version; }

    virtual std::size_t serialize(memory::ByteBuffer& buffer);

    bool findHeader(const std::string& name, std::string& value) const;
    HttpMessage& addHeader(const std::string& name, const std::string& value);
    HttpMessage& appendBody(const char *data, std::size_t size);

public:
    virtual std::string getField(FieldType type) const = 0;
    virtual void setField(FieldType type, std::string&& value) = 0;

    virtual void reset();

protected:
    HttpMessage();
    DEFAULT_VIRTUAL_DTOR(HttpMessage);

    virtual std::size_t serializeFirstLine(memory::ByteBuffer& buffer) = 0;

    std::size_t serializeHeaders(memory::ByteBuffer& buffer);

protected:
    int major_version_;
    int minor_version_;

private:
    std::map<std::string, std::string> headers_;
    memory::ByteBuffer body_;
    SerializeHelper helper_;

private:
    class SerializeHelper {
    public:
        SerializeHelper(HttpMessage& message);
        DEFAULT_DTOR(SerializeHelper);

        std::size_t serialize(memory::ByteBuffer& buffer);

        void reset();

    private:
        HttpMessage& message_;
        bool headers_serialized_;
        std::size_t body_serialized_;
    };

private:
    MAKE_NONCOPYABLE(HttpMessage);
};

} // namespace http
} // namespace message
} // namespace xproxy

#endif // HTTP_MESSAGE_HPP
