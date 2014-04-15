#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include "http_parser.h"

class Connection;
class HttpMessage;

class HttpParser {
public:
    template<class Buffer>
    std::size_t consume(const Buffer& buffer) {
        std::size_t consumed =
            ::http_parser_execute(&parser_, &settings_,
                                  buffer.data(), buffer.size());

        if (consumed != buffer.size()) {
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

    virtual ~HttpParser() = default;

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
    std::weak_ptr<Connection> connection_;

    http_parser parser_;

    bool header_completed_;
    bool message_completed_;
    bool chunked_;
    std::string current_header_field_;
    std::string current_header_value_;

    std::unique_ptr<HttpMessage> message_;

private:
    static http_parser_settings settings_;

    DISABLE_COPY_AND_ASSIGNMENT(HttpParser);
};

#endif // HTTP_PARSER_HPP
