#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "message/http/http_message.hpp"

namespace xproxy {
namespace message {
namespace http {

class HttpResponse : public HttpMessage {
public:
    DEFAULT_CTOR_AND_DTOR(HttpResponse);

    virtual std::string getField(FieldType type) const;

    virtual void setField(FieldType type, std::string&& value);

    virtual void reset();

protected:
    virtual std::size_t serializeFirstLine(memory::ByteBuffer& buffer);

private:
    int status_;
    std::string message_;
};

} // namespace http
} // namespace message
} // namespace xproxy

#endif // HTTP_RESPONSE_HPP
