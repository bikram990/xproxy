#ifndef HTTP_REQUEST_NITIAL_H
#define HTTP_REQUEST_NITIAL_H

#include "http_initial.h"

class HttpRequestInitial : public HttpInitial {
public:
    virtual ~HttpRequestInitial() {}

private:
    virtual void UpdateLineString() {
        std::stringstream ss;
        ss << method_ << ' ' << uri_ << ' '
           << "HTTP/" << major_version_ << '.' << minor_version_;
        line_ = ss.str();
    }

    std::string method_;
    std::string uri_;
    int major_version_;
    int minor_version_;
};

#endif // HTTP_REQUEST_INITIAL_H
