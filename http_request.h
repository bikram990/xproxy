#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <vector>
#include <string>
#include "http_header.h"
#include "log.h"


class HttpRequest {
public:
    enum State {
        kComplete = 0, // request is complete
        kEmptyRequest,
        kIncomplete, // request is incomplete, need to read more data from socket
        kBadRequest
    };

    static State BuildRequest(char *buffer, std::size_t length,
                              HttpRequest& request);

    HttpRequest() : buffer_built_(false), state_(kEmptyRequest), port_(80),
        method_("GET"), major_version_(1), minor_version_(1), body_length_(0) {
        TRACE_THIS_PTR;
    }
    ~HttpRequest() { TRACE_THIS_PTR; }

    boost::asio::streambuf& OutboundBuffer();

    const std::string& host() const { return host_; }
    short port() const { return port_; }

    HttpRequest& method(const std::string& method) {
        method_ = method;
        return *this; // return self for chaining operation
    }
    HttpRequest& uri(const std::string& uri) {
        uri_ = uri;
        return *this;
    }
    HttpRequest& major_version(int major_version) {
        major_version_ = major_version;
        return *this;
    }
    HttpRequest& minor_version(int minor_version) {
        minor_version_ = minor_version;
        return *this;
    }
    HttpRequest& body_length(std::size_t length) {
        body_length_ = length;
        return *this;
    }
    HttpRequest& AddHeader(const std::string& name, const std::string& value) {
        headers_.push_back(HttpHeader(name, value));
        return *this;
    }

    template<typename IterT>
    HttpRequest& body(IterT begin, IterT end) {
        body_ = std::string(begin, end);
        body_length_ = body_.length();
        return *this;
    }

private:
    struct HeaderFinder {
        std::string name;
        HeaderFinder(const std::string& name) : name(name) {}
        bool operator()(const HttpHeader& header) {
            return header.name == name;
        }
    };

    State ConsumeInitialLine(const std::string& line);
    State ConsumeHeaderLine(const std::string& line);

    const std::string& FindHeader(const std::string& name);

    bool buffer_built_; // to indicate whether the raw buffer is built
    State state_;

    std::string host_;
    short port_;

    boost::asio::streambuf raw_buffer_;

    std::string method_;
    std::string uri_;
    int major_version_;
    int minor_version_;
    std::vector<HttpHeader> headers_;
    std::size_t body_length_;
    std::string body_;
};

#endif // HTTP_REQUEST_H
