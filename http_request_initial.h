#ifndef HTTP_REQUEST_NITIAL_H
#define HTTP_REQUEST_NITIAL_H

#include "http_initial.h"

class HttpRequestInitial : public HttpInitial {
public:
    HttpResponseInitial()
        : HttpInitial(), method_("GET") {}

    virtual ~HttpRequestInitial() {}

    std::string& method() {
        modified_ = true;
        return method_;
    }

    const std::string& method() const {
        return method_;
    }

    std::string& uri() {
        modified_ = true;
        return uri_;
    }

    const std::string& uri() const {
        return uri_;
    }

private:
    virtual void UpdateLineString() {
        std::stringstream ss;
        ss << method_ << ' ' << uri_ << ' '
           << "HTTP/" << major_version_ << '.' << minor_version_;
        line_ = ss.str();
    }

    std::string method_;
    std::string uri_;
};

#endif // HTTP_REQUEST_INITIAL_H
