#ifndef HTTP_HEADERS_H
#define HTTP_HEADERS_H

#include <vector>
#include "http_object.h"

struct HttpHeader {
    std::string name;
    std::string value;

    HttpHeader(const std::string& name = "",
               const std::string& value = "")
        : name(name), value(value) {}
};

class HttpHeaders : public HttpObject {
public:
    virtual ~HttpHeaders() {}

    bool empty() const { return headers_.empty(); }

    void PushBack(HttpHeader&& header) {
        headers_.push_back(header);
        modified_ = true;
    }

    HttpHeader& back() {
        modified_ = true;   // we should assume the header will be modified here
        return headers_.back();
    }

    const HttpHeader& back() const {
        return headers_.back();
    }

    bool find(const std::string& name, std::string& value) const {
        auto it = std::find_if(headers_.begin(), headers_.end(),
                               boost::bind(&HttpHeaders::match, name, _1));
        if(it == headers_.end())
            return false;
        value = it->value;
        return true;
    }

private:
    virtual void UpdateByteBuffer() {
        content_->reset();
        for(auto it = headers_.begin(); it != headers_.end(); ++it)
            *content_ << it->name << ": " << it->value << "\r\n";

        *content_ << "\r\n";
    }

    bool match(const std::string& desired, const HttpHeader& actual) {
        return desired == actual.name;
    }

    std::vector<HttpHeader> headers_;
};

#endif // HTTP_HEADERS_H
