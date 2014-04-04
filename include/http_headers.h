#ifndef HTTP_HEADERS_H
#define HTTP_HEADERS_H

#include <vector>
#include "common.h"

struct HttpHeader {
    std::string name;
    std::string value;

    HttpHeader(const std::string& name = "",
               const std::string& value = "")
        : name(name), value(value) {}
};

class HttpHeaders {
public:
    DEFAULT_CTOR_AND_DTOR(HttpHeaders);

    bool empty() const { return headers_.empty(); }

    void add(HttpHeader&& header) { headers_.push_back(header); }

    void reset();

    bool find(const std::string& name, std::string& value) const;

    bool serialize(boost::asio::streambuf& out_buffer);

private:
    std::vector<HttpHeader> headers_;

private:
    DISABLE_COPY_AND_ASSIGNMENT(HttpHeaders);
};

#endif // HTTP_HEADERS_H
