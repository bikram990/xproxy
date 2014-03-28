#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "http_message.h"

class HttpRequest : public HttpMessage {
public:
    HttpRequest(std::shared_ptr<Connection> connection);
    virtual ~HttpRequest() = default;

    virtual void reset();

    virtual bool serialize(boost::asio::streambuf& out_buffer);

    virtual int OnUrl(const char *at, std::size_t length);

private:
    std::string method_;
    std::string url_;
};

#endif // HTTP_REQUEST_H
