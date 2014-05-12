#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include "message/http/http_message.hpp"

namespace xproxy {
namespace message {
namespace http {

class HttpRequest : public HttpMessage {
public:
    DEFAULT_CTOR_AND_DTOR(HttpRequest);

    virtual std::string getField(FieldType type) const;

    virtual void setField(FieldType type, std::string&& value);

    virtual void reset();

protected:
    virtual std::size_t serializeFirstLine(memory::ByteBuffer& buffer);

private:
    std::string method_;
    std::string uri_;
};

} // namespace xproxy
} // namespace message
} // namespace http

#endif // HTTP_REQUEST_HPP
