#ifndef HTTP_MESSAGE_HPP
#define HTTP_MESSAGE_HPP

#include <map>
#include "x/common.hpp"
#include "x/memory/byte_buffer.hpp"
#include "x/message/message.hpp"

namespace x {
namespace message {
namespace http {

class http_message : public message {
public:
    DEFAULT_DTOR(http_message);

    virtual bool deliverable() = 0;

    virtual void reset() {
        major_version_ = 0;
        minor_version_ = 0;
        headers_completed_ = false;
        message_completed_ = false;
        headers_.clear();
        body_.clear();
    }

    virtual bool completed() const {
        return message_completed_;
    }

    bool headers_completed() const {
        return headers_completed_;
    }

    void headers_completed(bool completed) {
        headers_completed_ = completed;
    }

    void message_completed(bool completed) {
        message_completed_ = completed;
    }

    int get_major_version() const {
        return major_version_;
    }

    int get_minor_version() const {
        return minor_version_;
    }

    void set_major_version(int version) {
        major_version_ = version;
    }

    void set_minor_version(int version) {
        minor_version_ = version;
    }

    bool find_header(const std::string& name, std::string& value) const {
        for (auto it = headers_.begin(); it != headers_.end(); ++it) {
            if (it->first == name) {
                value = it->second;
                return true;
            }
        }

        return false;
    }

    http_message& add_header(const std::string& name, const std::string& value) {
        headers_.insert(std::make_pair(name, value));
        return *this;
    }

    const std::map<std::string, std::string>& get_headers() const {
        return headers_;
    }

    std::map<std::string, std::string>& get_headers() {
        return headers_;
    }

    http_message& append_body(const char *data, std::size_t size) {
        body_ << memory::byte_buffer::wrap(data, size);
        return *this;
    }

    http_message& append_body(const std::string& str) {
        body_ << str;
        return *this;
    }

    const memory::byte_buffer& get_body() const {
        return body_;
    }

    memory::byte_buffer& get_body() {
        return body_;
    }

protected:
    http_message()
        : major_version_(0),minor_version_(0),
          headers_completed_(false), message_completed_(false) {}

    int major_version_;
    int minor_version_;

private:
    bool headers_completed_;
    bool message_completed_;
    std::map<std::string, std::string> headers_;
    memory::byte_buffer body_;

    MAKE_NONCOPYABLE(http_message);
};

} // namespace http
} // namespace message
} // namespace x

#endif // HTTP_MESSAGE_HPP
