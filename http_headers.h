#ifndef HTTP_HEADERS_H
#define HTTP_HEADERS_H

#include <vector>
#include "common.h"
#include "serializable.h"

struct HttpHeader {
    std::string name;
    std::string value;

    HttpHeader(const std::string& name = "",
               const std::string& value = "")
        : name(name), value(value) {}
};

class HttpHeaders : public Serializable {
public:
    DEFAULT_CTOR_AND_DTOR(HttpHeaders);

    bool empty() const { return headers_.empty(); }

    void PushBack(HttpHeader&& header);

    HttpHeader& back();

    const HttpHeader& back() const;

    void reset();

    bool find(const std::string& name, std::string& value) const;

    virtual bool serialize(boost::asio::streambuf& out_buffer);

private:
    DISABLE_COPY_AND_ASSIGNMENT(HttpHeaders);

    bool match(const std::string& desired, const HttpHeader& actual) const;

private:
    std::vector<HttpHeader> headers_;
};

typedef std::shared_ptr<HttpHeaders> HttpHeadersPtr;

#endif // HTTP_HEADERS_H
