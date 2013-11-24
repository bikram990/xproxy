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

    virtual void UpdateInitialLine() {}

private:
    struct ResponseTemplate {
        std::string message;
        std::string body;
        ResponseTemplate() {}
        ResponseTemplate(const std::string& message, const std::string& body)
            : message(message), body(body) {}
    };

    static std::map<StatusCode, ResponseTemplate> status_messages_;
};

#endif // HTTP_RESPONSE_H
