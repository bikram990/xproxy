#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "http_message.h"

class HttpResponse : public HttpMessage {
public:
    HttpResponse();
    virtual ~HttpResponse();

    virtual void reset();

    virtual bool serialize(boost::asio::streambuf& out_buffer);

    virtual int Status(const char *at, std::size_t length);

private:
    unsigned int status_code_;
    std::string status_message_;
};

#endif // HTTP_RESPONSE_H
