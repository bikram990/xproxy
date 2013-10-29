#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <boost/asio.hpp>
#include "http_header.h"
#include "log.h"


class HttpRequest {
public:
    typedef boost::shared_ptr<HttpRequest> Ptr;

    enum State {
        kComplete = 0, // request is complete
        kIncomplete, // request is incomplete, need to read more data from socket
        kBadRequest
    };

    HttpRequest() : buffer_built_(false), state_(kIncomplete),
                    build_state_(kRequestStart), port_(80),
                    major_version_(1), minor_version_(1), body_length_(0) {
        TRACE_THIS_PTR;
    }
    ~HttpRequest() { TRACE_THIS_PTR; }

    State operator<<(boost::asio::streambuf& stream);
    void reset();
    boost::asio::streambuf& OutboundBuffer();

    const std::string& host() const { return host_; }
    short port() const { return port_; }
    const std::string& method() const { return method_; }

    HttpRequest& host(const std::string& host) {
        host_ = host;
        return *this;
    }
    HttpRequest& port(short port) {
        port_ = port;
        return *this;
    }
    HttpRequest& method(const std::string& method) {
        method_ = method;
        return *this;
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
    HttpRequest& AddHeader(const std::string& name, const std::string& value) {
        headers_.push_back(HttpHeader(name, value));
        return *this;
    }
    HttpRequest& body(const boost::asio::streambuf& buf) {
        std::size_t copied = boost::asio::buffer_copy(body_.prepare(buf.size()), buf.data());
        body_.commit(copied);
        return *this;
    }

private:
    enum BuildState {
        kRequestStart, kMethod, kUri, kProtocolH, kProtocolT1, kProtocolT2, kProtocolP,
        kSlash, kMajorVersionStart, kMajorVersion, kMinorVersionStart, kMinorVersion,
        kNewLineHeader, kHeaderStart, kNewLineBody, kHeaderLWS, // linear white space
        kHeaderName, kNewLineHeaderContinue, kHeaderValue, kHeaderValueSpaceBefore,
        kHeadersDone
    };

    struct HeaderFinder {
        std::string name;
        HeaderFinder(const std::string& name) : name(name) {}
        bool operator()(const HttpHeader& header) {
            return header.name == name;
        }
    };

    bool FindHeader(const std::string& name, std::string& value);
    void CanonicalizeUri();

    State consume(char current_byte);
    bool ischar(int c);
    bool istspecial(int c);

    bool buffer_built_; // to indicate whether the raw buffer is built
    boost::asio::streambuf raw_buffer_;

    State state_;
    BuildState build_state_;

    std::string host_;
    short port_;
    std::string method_;
    std::string uri_;
    int major_version_;
    int minor_version_;
    std::vector<HttpHeader> headers_;
    boost::asio::streambuf body_;
    std::size_t body_length_;
};

#endif // HTTP_REQUEST_H
