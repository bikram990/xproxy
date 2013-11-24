#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>
#include "http_response.h"

std::map<HttpResponse::StatusCode, HttpResponse::ResponseTemplate>
HttpResponse::status_messages_ =
    boost::assign::map_list_of<HttpResponse::StatusCode, HttpResponse::ResponseTemplate>
(HttpResponse::kBadRequest, ResponseTemplate("Bad Request", "This is a bad request"));

void HttpResponse::StockResponse(StatusCode status, HttpResponse& response) {
    StockResponse(status, status_messages_[status].message, status_messages_[status].body, response);
}

void HttpResponse::StockResponse(StatusCode status, const std::string& message,
                                 const std::string& body, HttpResponse& response) {
    response.initial_line_ = "HTTP/1.1 " + boost::lexical_cast<std::string>(static_cast<int>(status)) + " " + message;
    std::ostream out(&response.body_);
    out << body;
    response.body_length_ = response.body_.size();

    response.headers_.push_back(HttpHeader("Proxy-Connection", "Keep-Alive"));
    response.headers_.push_back(HttpHeader("Content-Length", boost::lexical_cast<std::string>(response.body_length_)));
}
