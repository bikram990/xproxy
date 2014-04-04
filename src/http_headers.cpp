#include "http_headers.h"

void HttpHeaders::reset() {
    headers_.clear();
}

bool HttpHeaders::find(const std::string& name, std::string& value) const {
    auto it = std::find_if(headers_.begin(), headers_.end(),
                           [&](const HttpHeader& actual) {
                               return name == actual.name;
                           });
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
