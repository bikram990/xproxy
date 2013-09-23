#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <vector>
#include <string>
#include "http_header.h"


class HttpRequest {
public:
    enum BuildResult {
        kBadRequest = 0,
        kNotComplete, // the request is incomplete, need to read more data from socket
        kComplete // the request is complete
    };

    HttpRequest();
    ~HttpRequest();
    BuildResult BuildRequest(char *buffer, std::size_t length);

    boost::asio::streambuf& OutboundBuffer();

    const std::string& host() const { return host_; }
    short port() const { return port_; }

private:
    enum BuildState {
        kRequestStart,
        kMethod,
        kUri,
        kProtocolH,
        kProtocolT1,
        kProtocolT2,
        kProtocolP,
        kSlash,
        kMajorVersionStart,
        kMajorVersion,
        kMinorVersionStart,
        kMinorVersion,
        kNewLineHeader,
        kHeaderStart,
        kNewLineBody,
        kHeaderLWS, // linear white space
        kHeaderName,
        kNewLineHeaderContinue,
        kHeaderValue,
        kHeaderValueSpaceBefore
    };

    struct HeaderFinder {
        std::string name;
        HeaderFinder(const char *name) : name(name) {}
        bool operator()(const HttpHeader& header) {
            return header.name == name;
        }
    };

    BuildResult ConsumeInitialLine(const std::string& line);
    BuildResult ConsumeHeaderLine(const std::string& line);
    BuildResult consume(char current_byte);
    bool ischar(int c);
    bool istspecial(int c);

    const std::string FindHeader(const char *name);

    BuildState state_;

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
