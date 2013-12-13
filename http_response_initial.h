#ifndef HTTP_RESPONSE_NITIAL_H
#define HTTP_RESPONSE_NITIAL_H

#include "http_initial.h"

class HttpResponseInitial : public HttpInitial {
public:
    HttpResponseInitial()
        : HttpInitial(), status_code_(200), status_message_("OK") {}

    virtual ~HttpResponseInitial() {}

    int GetStatusCode() const { return status_code_; }

    void SetStatusCode(int code) {
        modified_ = true;
        status_code_ = code;
    }

    std::string& StatusMessage() {
        modified_ = true;
        return status_message_;
    }

    const std::string& StatusMessage() const {
        return status_message_;
    }

private:
    virtual void UpdateLineString() {
        std::stringstream ss;
        ss << "HTTP/" << major_version_ << '.' << minor_version_
           << ' ' << status_code_ << ' ' << status_message_;
        line_ = ss.str();
    }

    int status_code_;
    std::string status_message_;
};

#endif // HTTP_RESPONSE_INITIAL_H
