#include "http_request.h"

HttpRequest::HttpRequest() : HttpMessage(HTTP_REQUEST) {}

HttpRequest::~HttpRequest() {}

void HttpRequest::reset() {
    HttpMessage::reset();
    method_.clear();
    url_.clear();
}

bool HttpRequest::serialize(boost::asio::streambuf& out_buffer) {
    // TODO handle the condition that the message is incomplete
    {
        std::ostream out(&out_buffer);
        out << method_ << ' ' << url_ << "HTTP/"
            << MajorVersion() << '.' << MinorVersion()
            << "\r\n";
    }
    headers_.serialize(out_buffer);
    boost::asio::buffer_copy(out_buffer.prepare(body_.size()),
                             body_);
}

int HttpRequest::OnUrl(const char *at, std::size_t length) {
    method_ = http_method_str(parser_.method);
    // TODO can this pass compilation?
    url_ = std::forward(std::string(at, length));
    return 0;
}
