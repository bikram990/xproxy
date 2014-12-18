#include "x/codec/http/http_decoder.hpp"
#include "x/log/log.hpp"
#include "x/message/http/http_message.hpp"
#include "x/message/http/http_request.hpp"
#include "x/message/http/http_response.hpp"

namespace x {
namespace codec {
namespace http {

http_parser_settings http_decoder::settings_ = {
    &http_decoder::on_message_begin,
    &http_decoder::on_url,
    &http_decoder::on_status,
    &http_decoder::on_header_field,
    &http_decoder::on_header_value,
    &http_decoder::on_headers_complete,
    &http_decoder::on_body,
    &http_decoder::on_message_complete
};

std::size_t http_decoder::decode(const char *begin, std::size_t length, message::message& msg) {
    message_ = dynamic_cast<message::http::http_message *>(&msg);
    assert(message_);

    auto consumed = ::http_parser_execute(&parser_, &settings_, begin, length);

    if (consumed != length) {
        if (HTTP_PARSER_ERRNO(&parser_) != HPE_OK) {
            XERROR << "decode error, code: " << parser_.http_errno
                   << ", message: " << error_message() << '\n'
                   << "----- error message dump begin -----\n"
                   << std::string(begin, length)
                   << "\n-----  error message dump end  -----";
            return 0;
        } else {
            #warning TODO will this happen? a message is parsed, but there is still data?
            XWARN << "Weird: parser does not consume all data, but there is no error.";
            return consumed;
        }
    }

    return consumed;
}

bool http_decoder::keep_alive() const {
    if (!headers_completed_) // return false when headers are incomplete
        return false;
    return ::http_should_keep_alive(const_cast<http_parser*>(&parser_)) != 0;
}

void http_decoder::reset() {
    ::http_parser_init(&parser_, static_cast<http_parser_type>(parser_.type));
    message_ = nullptr;
    headers_completed_ = false;
    message_completed_ = false;
    chunked_ = false;
    current_header_field_.clear();
    current_header_value_.clear();
}

int http_decoder::on_message_begin(http_parser *parser) {
    auto p = static_cast<http_decoder*>(parser->data);
    assert(p);
    assert(p->message_);

    p->headers_completed_ = false;
    p->message_completed_ = false;
    p->chunked_ = false;
    p->current_header_field_.clear();
    p->current_header_value_.clear();
    return 0;
}

int http_decoder::on_url(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<http_decoder*>(parser->data);
    assert(p);
    assert(p->message_);

    auto method = ::http_method_str(static_cast<http_method>(parser->method));
    auto request = dynamic_cast<message::http::http_request *>(p->message_);
    assert(request);

    request->set_method(std::string(method));

    // Note: a request sent proxy contains the first line as below:
    //       => GET http://example.com/some/resource HTTP/1.1
    // so, we should convert it into the following before sending it
    // to server:
    //       => GET /some/resource HTTP/1.1
    std::string uri(at, length);
    if (parser->method != HTTP_CONNECT) {
        if (uri[0] != '/') {
            const static std::string http("http://");
            auto end = std::string::npos;
            if(uri.compare(0, http.length(), http) != 0)
                end = uri.find('/');
            else
                end = uri.find('/', http.length());

            if(end == std::string::npos) {
                uri = '/';
            } else {
                uri.erase(0, end);
            }
        }
    }

    request->set_uri(uri);
    return 0;
}

int http_decoder::on_status(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<http_decoder*>(parser->data);
    assert(p);
    assert(p->message_);

    auto response = dynamic_cast<message::http::http_response *>(p->message_);
    assert(response);

    response->set_status(p->parser_.status_code);
    response->set_message(std::string(at, length));

    return 0;
}

int http_decoder::on_header_field(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<http_decoder*>(parser->data);
    assert(p);
    assert(p->message_);

    if (!p->current_header_value_.empty()) {
        p->message_->add_header(p->current_header_field_, p->current_header_value_);
        p->current_header_field_.clear();
        p->current_header_value_.clear();
    }
    p->current_header_field_.append(at, length);
    return 0;
}

int http_decoder::on_header_value(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<http_decoder*>(parser->data);
    assert(p);
    assert(p->message_);

    p->current_header_value_.append(at, length);
    return 0;
}

int http_decoder::on_headers_complete(http_parser *parser) {
    auto p = static_cast<http_decoder*>(parser->data);
    assert(p);
    assert(p->message_);

    // current_header_value_ MUST NOT be empty
    assert(!p->current_header_value_.empty());

    p->message_->set_major_version(parser->http_major);
    p->message_->set_minor_version(parser->http_minor);

    p->message_->add_header(p->current_header_field_, p->current_header_value_);
    p->current_header_field_.clear();
    p->current_header_value_.clear();

    p->headers_completed_ = true;
    p->message_->headers_completed(true);

    if (p->parser_.flags & F_CHUNKED)
        p->chunked_ = true;

    return 0;
}

int http_decoder::on_body(http_parser *parser, const char *at, std::size_t length) {
    auto p = static_cast<http_decoder*>(parser->data);
    assert(p);
    assert(p->message_);

    if (p->chunked_) {
        std::ostringstream out;
        out << std::hex << length;
        out.copyfmt(std::ios(nullptr));
        out << CRLF;
        p->message_->append_body(out.str());
    }
    p->message_->append_body(at, length);
    if (p->chunked_) {
        p->message_->append_body(CRLF, 2);
    }

    return 0;
}

int http_decoder::on_message_complete(http_parser *parser) {
    auto p = static_cast<http_decoder*>(parser->data);
    assert(p);
    assert(p->message_);

    if (p->chunked_) {
        p->message_->append_body(END_CHUNK, 5);
    }

    p->message_completed_ = true;
    p->message_->message_completed(true);

    return 0;
}

} // namespace http
} // namespace codec
} // namespace x
