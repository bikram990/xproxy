#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include "x/message/http/http_message.hpp"

namespace x {
namespace message {
namespace http {

class http_request : public http_message {
public:
    DEFAULT_CTOR(http_request);
    DEFAULT_DTOR(http_request);

    virtual bool deliverable() const {
        return completed();
    }

    virtual void reset() {
        http_message::reset();
        method_.clear();
        uri_.clear();
    }

    std::string get_method() const {
        return method_;
    }

    void set_method(const std::string& method) {
        method_ = method;
    }

    std::string get_uri() const {
        return uri_;
    }

    void set_uri(const std::string& uri) {
        uri_ = uri;
    }

private:
    std::string method_;
    std::string uri_;

    MAKE_NONCOPYABLE(http_request);
};

} // namespace http
} // namespace message
} // namespace x

#endif // HTTP_REQUEST_HPP
