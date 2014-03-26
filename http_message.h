#ifndef HTTP_MESSAGE_H
#define HTTP_MESSAGE_H

#include "common.h"
#include "http_headers.h"
#include "http_parser.h"
#include "serializable.h"

class HttpMessage : public Serializable, public HttpParser {
public:
    virtual bool serialize(boost::asio::streambuf& out_buffer) = 0;

    virtual void reset();

    virtual int OnMessageBegin();

    // this method should be overridden in HttpRequest
    virtual int OnUrl(const char *at, std::size_t length);

    // this method should be overridden in HttpResponse
    virtual int OnStatus(const char *at, std::size_t length);

    virtual int OnHeaderField(const char *at, std::size_t length);

    virtual int OnHeaderValue(const char *at, std::size_t length);

    virtual int OnHeadersComplete();

    virtual int OnBody(const char *at, std::size_t length);

    virtual int OnMessageComplete();

    bool completed() const { return completed_; }

    int MajorVersion() const { return parser_.http_major; }

    int MinorVersion() const { return parser_.http_minor; }

protected:
    HttpMessage(http_parser_type type);
    virtual ~HttpMessage();

protected:
    bool completed_;
    std::string current_header_field_; // store temp header name/value during parsing
    std::string current_header_value_;

    HttpHeaders headers_;
    boost::asio::streambuf body_;

private:
    DISABLE_COPY_AND_ASSIGNMENT(HttpMessage);
};

#endif // HTTP_MESSAGE_H
