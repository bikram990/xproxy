#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <boost/asio.hpp>
#include "http_header.h"
#include "log.h"


class HttpResponse {
public:
    enum StatusCode {
        kBadRequest = 400
        // TODO currently the only one is enough, may add others later
    };

    static void StockResponse(StatusCode status, HttpResponse& response);
    static void StockResponse(StatusCode status, const std::string& message,
                              const std::string& body, HttpResponse& response);

    HttpResponse() : buffer_built_(false), body_length_(0) { TRACE_THIS_PTR; }
    ~HttpResponse() { TRACE_THIS_PTR; }

    boost::asio::streambuf& OutboundBuffer();

    std::string& status_line() { return status_line_; }
    boost::asio::streambuf& body() { return body_; }

    HttpResponse& AddHeader(const std::string& name, const std::string& value) {
        headers_.push_back(HttpHeader(name, value));
        return *this; // for chaining operation
    }

    void body_lenth(std::size_t length) {
        body_length_ = length;
    }

    std::size_t body_length() { return body_length_; }

private:
    struct ResponseTemplate {
        std::string message;
        std::string body;
    };

    static std::map<StatusCode, ResponseTemplate> status_messages_;

    bool buffer_built_;
    boost::asio::streambuf raw_buffer_;

    std::string status_line_;
    std::vector<HttpHeader> headers_;
    boost::asio::streambuf body_;
    std::size_t body_length_;
};

#endif // HTTP_RESPONSE_H
