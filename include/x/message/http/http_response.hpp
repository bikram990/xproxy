#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "x/common.hpp"
#include "x/message/http/http_message.hpp"

namespace x {
namespace message {
namespace http {

class http_response: public http_message {
public:
    enum response_type {
        SSL_REPLY
    };

    static std::shared_ptr<http_response> make_response(response_type type) {
        std::shared_ptr<http_response> response = std::make_shared<http_response>();
        switch(type) {
        case SSL_REPLY: {
            response->set_major_version(1);
            response->set_minor_version(1);
            response->set_status(200);
            response->set_message("Connection Established");
            response->add_header("Content-Length", "0");
            response->add_header("Connection","keep-alive");
            response->add_header("Proxy-Connection", "keep-alive");
            return response;
        }
        default:
            assert(0);
        }
    }

    DEFAULT_DTOR(http_response);

    http_response() : status_(0) {}

    virtual bool deliverable() const {
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
