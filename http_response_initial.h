#ifndef HTTP_RESPONSE_NITIAL_H
#define HTTP_RESPONSE_NITIAL_H

#include "http_initial.h"

class HttpResponseInitial : public HttpInitial {
public:
    virtual ~HttpResponseInitial() {}

private:
    virtual void UpdateLineString() {
        std::stringstream ss;
        ss << "HTTP/" << major_version_ << '.' << minor_version_
           << ' ' << status_code_ << ' ' << status_message_;
        line_ = ss.str();
    }

    int major_version_;
    int minor_version_;
    int status_code_;
    std::string status_message_;
};

#endif // HTTP_RESPONSE_INITIAL_H
