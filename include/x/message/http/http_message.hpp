#ifndef HTTP_MESSAGE_HPP
#define HTTP_MESSAGE_HPP

namespace x {
namespace message {
namespace http {

class http_message : public message {
public:
    DEFAULT_DTOR(message);

    virtual bool deliverable() = 0;

    virtual http_parser_type get_decoder_type() = 0;

    virtual void reset();

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

    http_message& append_body(const char *data, std::size_t size) {
        body_ << memory::byte_buffer::wrap(data, size);
        return *this;
    }

protected:
    int major_version_;
    int minor_version_;

private:
    std::map<std::string, std::string> headers_;
    memory::byte_buffer body_;

    MAKE_NONCOPYABLE(http_message);
};

} // namespace http
} // namespace message
} // namespace x

#endif // HTTP_MESSAGE_HPP
