#include "connection.h"
#include "http_request.h"

HttpRequest::HttpRequest(std::shared_ptr<Connection> connection)
    : HttpMessage(connection, HTTP_REQUEST) {}

void HttpRequest::reset() {
    HttpMessage::reset();
    method_.clear();
    url_.clear();
}

bool HttpRequest::serialize(boost::asio::streambuf& out_buffer) {
    // for request, we only allow serialize when it is completed
    if (!MessageCompleted())
        return false;

    std::ostream out(&out_buffer);
    out << method_ << ' ' << url_ << " HTTP/"
        << MajorVersion() << '.' << MinorVersion()
        << "\r\n";

    headers_.serialize(out_buffer);

    if (body_.size() > 0)
        boost::asio::buffer_copy(out_buffer.prepare(body_.size()),
                                 boost::asio::buffer(body_.data(), body_.size()));
    return true;
}

int HttpRequest::OnUrl(const char *at, std::size_t length) {
    method_ = ::http_method_str(static_cast<http_method>(parser_.method));
    // TODO can this pass compilation?
    url_ = std::move(std::string(at, length));
    return 0;
}
