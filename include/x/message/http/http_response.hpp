#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "x/common.hpp"
#include "x/message/http/http_message.hpp"

namespace x {
namespace message {
namespace http {

class http_response: public http_message {
public:
    DEFAULT_DTOR(http_response);

    http_response() : status_(0) {}

    virtual bool deliverable() {
        return true;
    }

    virtual void reset() {
        http_message::reset();
        status_ = 0;
        message_.clear();
    }

    unsigned int get_status() const {
        return status_;
    }

    void set_status(unsigned int status) {
        status_ = status;
    }

    std::string get_message() const {
        return message_;
    }

    void set_message(const std::string& message) {
        message_ = message;
    }

private:
    unsigned int status_;
    std::string message_;

    MAKE_NONCOPYABLE(http_response);
};

} // namespace http
} // namespace message
} // namespace x

#endif // HTTP_RESPONSE_HPP
