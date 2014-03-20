#ifndef HTTP_MESSAGE_H
#define HTTP_MESSAGE_H

#include "common.h"
#include "http_headers.h"

class HttpMessage {
public:
    HttpMessage();
    virtual ~HttpMessage() {}

    virtual void reset() = 0;

protected:
    HttpHeaders headers_;
    boost::asio::streambuf body_;

private:
    DISABLE_COPY_AND_ASSIGNMENT(HttpMessage);
};

#endif // HTTP_MESSAGE_H
