#include <boost/bind.hpp>
#include "http_headers.h"

void HttpHeaders::PushBack(HttpHeader&& header) {
    headers_.push_back(header);
    //modified_ = true;
}

HttpHeader& HttpHeaders::back() {
    //modified_ = true;   // we should assume the header will be modified here
    return headers_.back();
}

const HttpHeader& HttpHeaders::back() const {
    return headers_.back();
}

void HttpHeaders::reset() {
    headers_.clear();
}

bool HttpHeaders::find(const std::string& name, std::string& value) {
    auto it = std::find_if(headers_.begin(), headers_.end(),
                           boost::bind(&HttpHeaders::match, this, name, _1));
    if(it == headers_.end())
        return false;
    value = it->value;
    return true;
}

bool HttpHeaders::serialize(boost::asio::streambuf& out_buffer) {
    std::ostream out(&out_buffer);
    for (auto it = headers_.begin(); it != headers_.end(); ++it) {
        out << it->name << ": " << it->value << "\r\n";
    }
    out << "\r\n";

    return true;
}

bool HttpHeaders::match(const std::string& desired, const HttpHeader& actual) {
    return desired == actual.name;
}
