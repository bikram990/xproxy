#include "http_response.h"


HttpResponse::HttpResponse() : HttpMessage(HTTP_RESPONSE) {}

HttpResponse::~HttpResponse() {}

void HttpResponse::reset() {
    HttpMessage::reset();
    status_code_ = 0;
    status_message_.clear();
}

bool HttpResponse::serialize(boost::asio::streambuf& out_buffer) {
    // TODO handle the condition that the message is incomplete
    {
        std::ostream out(&out_buffer);
        out << "HTTP/" << MajorVersion() << '.' << MinorVersion()
            << ' ' << status_code_ << ' ' << status_message_
            << "\r\n";
    }
    headers_.serialize(out_buffer);
    boost::asio::buffer_copy(out_buffer.prepare(body_.size()),
                             body_);
}

int HttpResponse::OnStatus(const char *at, std::size_t length) {
    status_code_ = parser_.status_code;
    status_message_ = std::string(at, length);
    return 0;
}
