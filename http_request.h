#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "http_message.h"

class HttpRequest : public HttpMessage {
public:
    HttpRequest();
    virtual ~HttpRequest();

    virtual void reset();

    virtual bool serialize(boost::asio::streambuf& out_buffer);

    virtual int Url(const char *at, std::size_t length);

private:
    std::string method_;
    std::string url_;
};

#endif // HTTP_REQUEST_H
