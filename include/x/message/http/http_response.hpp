#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

namespace x {
namespace message {
namespace http {

class http_response: public http_message {
public:
    DEFAULT_DTOR(http_response);

    virtual bool deliverable() {
        return true;
    }

    virtual void reset();

    int get_status() const {
        return status_;
    }

    void set_status(int status) {
        status_ = status;
    }

    std::string get_message() const {
        return message_;
    }

    void set_message(const std::string& message) {
        message_ = message;
    }

private:
    int status_;
    std::string message_;

    MAKE_NONCOPYABLE(http_response);
};

} // namespace http
} // namespace message
} // namespace x

#endif // HTTP_RESPONSE_HPP
