#ifndef HTTP_PROXY_REQUEST_H
#define HTTP_PROXY_REQUEST_H

#include <map>


template<typename IterT>
class HttpProxyRequest : public Request<IterT> {
public:
    HttpProxyRequest();
    virtual ~HttpProxyRequest();
    virtual BuildResult BuildFromRaw(IterT begin, IterT end);

private:
    enum BuildState {
        kRequestStart,
        kMethod
        // TODO add more here...
    };

    typedef std::map<std::string, std::string> HeaderMap;

    BuildState state_;

    std::string host_;
    short port_;

    // the following contents are meaningless for a proxy
    std::string method_;
    std::string uri_;
    int major_version_;
    int minor_version_;
    HeaderMap headers_;
};

#endif // HTTP_PROXY_REQUEST_H
