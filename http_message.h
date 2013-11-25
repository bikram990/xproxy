#ifndef HTTP_MESSAGE_H
#define HTTP_MESSAGE_H

#include <boost/asio.hpp>
#include "http_header.h"

class HttpMessage {
private:
    struct HeaderFinder {
        std::string name;
        // TODO use move constructor to improve this
        HeaderFinder(const std::string& name) : name(name) {}
        bool operator()(const HttpHeader& header) {
            return header.name == name;
        }
    };

public:
    HttpMessage() : body_length_(0), buffer_built_(false) {}

    virtual ~HttpMessage() {}

    const std::string& InitialLine() const { return initial_line_; }

    bool FindHeader(const std::string& name, std::string& value) const {
        auto it = std::find_if(headers_.begin(), headers_.end(), HeaderFinder(name));
        if(it == headers_.end())
            return false;
        value = it->value;
        return true;
    }

    std::size_t BodyLength() { return body_length_; }

    boost::asio::streambuf& body() { return body_; }

    boost::asio::streambuf& InboundBuffer() { return raw_buffer_; }

    virtual void UpdateInitialLine() = 0;

    virtual boost::asio::streambuf& OutboundBuffer() {
        if(buffer_built_)
            return out_buffer_;

        std::ostream buf(&out_buffer_);

        UpdateInitialLine();

        buf << initial_line_ << "\r\n";

        for(auto it = headers_.begin(); it != headers_.end(); ++it)
            buf << it->name << ": " << it->value << "\r\n";

        buf << "\r\n";

        if(body_.size() > 0) {
            std::size_t copied = boost::asio::buffer_copy(out_buffer_.prepare(body_.size()), body_.data());
            out_buffer_.commit(copied);
        }

        buffer_built_ = true;
        return out_buffer_;
    }

    HttpMessage& AddHeader(const std::string& name, const std::string& value) {
        auto it = std::find_if(headers_.begin(), headers_.end(), HeaderFinder(name));
        if(it != headers_.end())
            it->value.replace(it->value.begin(), it->value.end(), value);
        else
            headers_.push_back(HttpHeader(name, value));
        return *this;
    }

    HttpMessage& SetBodyLength(std::size_t length) {
        body_length_ = length;
        return *this;
    }

    HttpMessage& body(const boost::asio::streambuf& buf) {
        std::size_t copied = boost::asio::buffer_copy(body_.prepare(buf.size()), buf.data());
        body_.commit(copied);
        return *this;
    }

    // void ResetHeaders() { headers_.clear(); }

    virtual void reset() {
        initial_line_.clear();
        headers_.clear();
        body_.consume(body_.size());
        body_length_ = 0;
        out_buffer_.consume(out_buffer_.size());
        buffer_built_ = false;
        raw_buffer_.consume(raw_buffer_.size());
    }

protected:
    std::string initial_line_;
    std::vector<HttpHeader> headers_;

    std::size_t body_length_;
    boost::asio::streambuf body_;

    bool buffer_built_;
    boost::asio::streambuf out_buffer_; // the buffer used to store modified message

    boost::asio::streambuf raw_buffer_; // the buffer used to store raw message
};

#endif // HTTP_MESSAGE_H
