#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>
#include "http_response.h"

std::map<enum HttpResponse::StatusCode, HttpResponse::ResponseTemplate>
HttpResponse::status_messages_ =
    boost::assign::map_list_of<enum HttpResponse::StatusCode, HttpResponse::ResponseTemplate>
(HttpResponse::kBadRequest, ResponseTemplate("Bad Request", "This is a bad request"));

void HttpResponse::StockResponse(enum StatusCode status, HttpResponse& response) {
    StockResponse(status, status_messages_[status].message, status_messages_[status].body, response);
}

void HttpResponse::StockResponse(enum StatusCode status, const std::string& message,
                                 const std::string& body, HttpResponse& response) {
    response.initial_line_ = "HTTP/1.1 " + boost::lexical_cast<std::string>(static_cast<int>(status)) + " " + message;
    std::ostream out(&response.body_);
    out << body;
    response.body_length_ = response.body_.size();

    response.headers_.push_back(HttpHeader("Proxy-Connection", "Keep-Alive"));
    response.headers_.push_back(HttpHeader("Content-Length", boost::lexical_cast<std::string>(response.body_length_)));
}

void HttpResponse::ConsumeInitialLine() {
    std::istream s(&raw_buffer_);
    s.ignore(5);
    char ignore = 0;
    s >> major_version_ >> ignore >> minor_version_ >> status_code_
      >> status_message_ >> ignore >> ignore; // the last two are crlf
}

void HttpResponse::ConsumeHeaders() {
    std::istream s(&raw_buffer_);
    std::string header;

    while(std::getline(s, header)) {
        if(header == "\r") // no more headers
            break;

        std::string::size_type sep_idx = header.find(':');
        if(sep_idx == std::string::npos) {
            XWARN << "Invalid header: " << header;
            continue;
        }

        std::string name = header.substr(0, sep_idx);
        std::string value = header.substr(sep_idx + 1, header.length() - 1 - name.length() - 1); // remove the last \r
        boost::algorithm::trim(name);
        boost::algorithm::trim(value);
        AddHeader(name, value);
    }
}

void HttpResponse::ConsumeBody(bool update_body_length) {
    std::size_t available = raw_buffer_.size();
    std::size_t copied = boost::asio::buffer_copy(body_.prepare(available), raw_buffer_.data());

    if(copied < available) {
        XERROR << "The copied size(" << copied << ") is less than available size("
               << available << "), but this should never happen.";
    }

    body_.commit(copied);
    raw_buffer_.consume(copied);

    if(update_body_length)
        body_length_ = body_.size();
}
