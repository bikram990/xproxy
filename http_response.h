#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <boost/asio.hpp>
#include "http_message.h"
#include "log.h"


class HttpResponse : public HttpMessage {
public:
    enum StatusCode {
        kBadRequest = 400
        // TODO currently the only one is enough, may add others later
    };

    static void StockResponse(StatusCode status, HttpResponse& response);
    static void StockResponse(StatusCode status, const std::string& message,
                              const std::string& body, HttpResponse& response);

    void ConsumeInitialLine();
    void ConsumeHeaders();
    void ConsumeBody(bool update_body_length = true);

    int StatusCode() const { return status_code_; }
    const std::string& StatusMessage() const { return status_message_; }

    virtual void UpdateInitialLine() {
        std::stringstream ss;
        ss << "HTTP/" << major_version_ << '.' << minor_version_
           << ' ' << status_code_ << ' ' << status_message_;
        initial_line_ = ss.str();
    }

private:
    struct ResponseTemplate {
        std::string message;
        std::string body;
        ResponseTemplate() {}
        ResponseTemplate(const std::string& message, const std::string& body)
            : message(message), body(body) {}
    };

    static std::map<enum StatusCode, ResponseTemplate> status_messages_;

    int major_version_;
    int minor_version_;
    int status_code_;
    std::string status_message_;
};

#endif // HTTP_RESPONSE_H
