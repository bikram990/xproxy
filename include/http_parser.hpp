#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include "common.h"
#include "http_parser.h"
#include "http_message.hpp"
#include "log.h"

class Connection;

class HttpParser {
public:
    template<class Buffer>
    std::size_t consume(const Buffer& buffer) {
        auto orig_size = message_->RawBuffer().size();
        message_->RawBuffer().append(boost::asio::buffer_cast<const char*>(buffer.data()), buffer.size(), false);

        std::size_t consumed =
            ::http_parser_execute(&parser_, &settings_,
                                  //message_->RawBuffer().data(), message_->RawBuffer().available());
                                  message_->RawBuffer().head() + orig_size, message_->RawBuffer().size() - orig_size);

        if (consumed != message_->RawBuffer().available()) {
            if (HTTP_PARSER_ERRNO(&parser_) != HPE_OK) {
                XERROR << "Error occurred during message parsing, error code: "
                       << parser_.http_errno << ", message: "
                       << ::http_errno_description(static_cast<http_errno>(parser_.http_errno));
                return 0;
            } else {
                // TODO will this happen? a message is parsed, but there is still data?
                XWARN << "Weird: parser does not consume all data, but there is no error.";
                return consumed;
            }
        }

        return consumed;
    }

    bool HeaderCompleted() const { return header_completed_; }

    bool MessageCompleted() const { return message_completed_; }

    HttpMessage& message() const { return *message_.get(); }

    bool KeepAlive() const;

    void reset();

public:
    HttpParser(std::shared_ptr<Connection> connection, http_parser_type type);

    virtual ~HttpParser();

    static int OnMessageBegin(http_parser *parser);
    static int OnUrl(http_parser *parser, const char *at, std::size_t length);
    static int OnStatus(http_parser *parser, const char *at, std::size_t length);
    static int OnHeaderField(http_parser *parser, const char *at, std::size_t length);
    static int OnHeaderValue(http_parser *parser, const char *at, std::size_t length);
    static int OnHeadersComplete(http_parser *parser);
    static int OnBody(http_parser *parser, const char *at, std::size_t length);
    static int OnMessageComplete(http_parser *parser);

private:
    void InitMessage(http_parser_type type);

private:
    struct buffer : public ByteBuffer::wrapper {
        bool empty() const { return dt == nullptr; }
        void clear() { dt = nullptr; sz = 0; }

        void append(const char *data, std::size_t size) {
            if (dt == nullptr) {
                dt = data;
                sz = size;
                return;
            }

            // to make sure it is continuous
            assert(data == dt + sz);

            sz += size;
        }

        operator std::string() const {
            return std::string(dt, sz);
        }
    };

private:
    std::weak_ptr<Connection> connection_;

    http_parser parser_;

    bool first_header_;
    bool header_completed_;
    bool message_completed_;
    bool chunked_;
    buffer current_header_field_;
    buffer current_header_value_;

    std::unique_ptr<HttpMessage> message_;

private:
    static http_parser_settings settings_;

    DISABLE_COPY_AND_ASSIGNMENT(HttpParser);
};

#endif // HTTP_PARSER_HPP
