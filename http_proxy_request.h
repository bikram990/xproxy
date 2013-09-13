#ifndef HTTP_PROXY_REQUEST_H
#define HTTP_PROXY_REQUEST_H

//#include <map>
#include <vector>
#include <string>
#include "request.h"


class HttpProxyRequest : public Request {
public:
    HttpProxyRequest();
    virtual ~HttpProxyRequest();
    virtual BuildResult BuildFromRaw(char *buffer, std::size_t length);

    const std::string& host() const { return host_; }
    short port() const { return port_; }

private:
    enum BuildState {
        kRequestStart,
        kMethod
        // TODO add more here...
    };

    struct Header {
        std::string name;
        std::string value;
    };

    //typedef std::map<std::string, std::string> HeaderMap;

    BuildState state_;

    std::string host_;
    short port_;

    // the following contents are meaningless for a proxy
    std::string method_;
    std::string uri_;
    int major_version_;
    int minor_version_;
    //HeaderMap headers_;
    std::vector<Header> headers_;
};

#endif // HTTP_PROXY_REQUEST_H
