#include "http_response.h"

HttpResponse::HttpResponse()
    : HttpMessage(HTTP_RESPONSE),
      status_code_(0),
      header_serialized_(false),
      body_serialized_position_(0) {}

void HttpResponse::reset() {
    HttpMessage::reset();
    status_code_ = 0;
    status_message_.clear();

    header_serialized_ = false;
    body_serialized_position_ = 0;
}

bool HttpResponse::serialize(boost::asio::streambuf& out_buffer) {
    // for response, we can serialize many times, but:
    // 1. the first operation must wait for header completed
    // 2. the later operations should only serialize body
    // TODO: find a better solution
    if (!HeaderCompleted())
        return false;

    if (!header_serialized_) {
        header_serialized_ = true;

        std::ostream out(&out_buffer);
        out << "HTTP/" << MajorVersion() << '.' << MinorVersion()
            << ' ' << status_code_ << ' ' << status_message_
            << "\r\n";
        headers_.serialize(out_buffer);

        if (body_.size() > 0)
            boost::asio::buffer_copy(out_buffer.prepare(body_.size()),
                                     boost::asio::buffer(body_.data(), body_.size()));
        body_serialized_position_ = body_.size();

        return true;
    }

    if (body_.size() - body_serialized_position_ > 0)
        boost::asio::buffer_copy(out_buffer.prepare(body_.size() - body_serialized_position_),
                                 boost::asio::buffer(body_.data() + body_serialized_position_, body_.size()));
    body_serialized_position_ = body_.size();
    return true;
}

int HttpResponse::OnStatus(const char *at, std::size_t length) {
    status_code_ = parser_.status_code;
    status_message_ = std::move(std::string(at, length));
    return 0;
}
