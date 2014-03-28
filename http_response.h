#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "http_message.h"

class HttpResponse : public HttpMessage {
public:
    HttpResponse(std::shared_ptr<Connection> connection);
    virtual ~HttpResponse() = default;

    virtual void reset();

    virtual bool serialize(boost::asio::streambuf& out_buffer);

    virtual int OnStatus(const char *at, std::size_t length);

private:
    unsigned int status_code_;
    std::string status_message_;

    // two helper members to help serialization
    bool header_serialized_;
    std::size_t body_serialized_position_;
};

#endif // HTTP_RESPONSE_H
