#include "connection.h"
#include "http_response.h"

HttpResponse::HttpResponse(std::shared_ptr<Connection> connection)
    : HttpMessage(connection, HTTP_RESPONSE),
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
    return serialize(out_buffer, false);
}

bool HttpResponse::serialize(boost::asio::streambuf& out_buffer, bool ignore_headers) {
    // for response, we can serialize many times, but:
    // 1. the first operation must wait for header completed
    // 2. the later operations should only serialize body
    // TODO: find a better solution
    if (!HeaderCompleted())
        return false;

    if (!header_serialized_ && !ignore_headers) {
        header_serialized_ = true;

        std::ostream out(&out_buffer);
        out << "HTTP/" << MajorVersion() << '.' << MinorVersion()
            << ' ' << status_code_ << ' ' << status_message_
            << "\r\n";
        headers_.serialize(out_buffer);
    }

    std::size_t actual_size = body_.size() - body_serialized_position_;
    if (actual_size > 0) {
        std::size_t copied = boost::asio::buffer_copy(out_buffer.prepare(actual_size),
                                                      boost::asio::buffer(body_.data() + body_serialized_position_,
                                                                          actual_size));
        assert(copied == actual_size);
        out_buffer.commit(copied);
        body_serialized_position_ += copied;
    }
    return true;
}

int HttpResponse::OnStatus(const char *at, std::size_t length) {
    status_code_ = parser_.status_code;
    status_message_ = std::move(std::string(at, length));
    return 0;
}
