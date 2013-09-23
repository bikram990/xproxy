#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <vector>
#include <string>
#include "http_header.h"


class HttpRequest {
public:
    enum BuildResult {
        kBuildError = 0,
        kNotStart, // TODO this is not used
        kNotComplete,
        kComplete
    };

    HttpRequest();
    ~HttpRequest();
    BuildResult BuildFromRaw(char *buffer, std::size_t length);

    boost::asio::streambuf& OutboundBuffer() { return raw_buffer_; }

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

    BuildResult consume(char current_byte);
    bool ischar(int c);
    bool istspecial(int c);

    BuildState state_;

    std::string host_;
    short port_;

    boost::asio::streambuf raw_buffer_;

    // the following contents are meaningless for a proxy
    std::string method_;
    std::string uri_;
    int major_version_;
    int minor_version_;
    //HeaderMap headers_;
    std::vector<HttpHeader> headers_;
    std::string body_;
};

#endif // HTTP_REQUEST_H
