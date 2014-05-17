#include "log.h"
#include "memory/byte_buffer.hpp"
#include "message/http/http_request.hpp"
#include "net/client_adapter.hpp"
#include "util/timer.hpp"

namespace xproxy {
namespace net {

ClientAdapter::ClientAdapter(Connection& connection)
    : connection_(connection),
      timer_(connection.service()),
      message_(new message::http::HttpRequest),
      parser_(new message::http::HttpParser(*message_, HTTP_REQUEST, this)),
      https_(false), ssl_built_(false) {}

void ClientAdapter::onConnect(const boost::system::error_code& e) {
    // do nothing here, a client connection is always connected
}

void ClientAdapter::onHandshake(const boost::system::error_code& e) {
    if (e) {
        XERROR_WITH_ID << "Handshake error: " << e.message();
        // TODO enhance
        connection_.stop();
        return;
    }

    ssl_built_ = true;
    message_->reset();
    parser_->reset();
    connection_.read();
}

void ClientAdapter::onRead(const boost::system::error_code &e, const char *data, std::size_t length) {
    if (e) {
        if(e == boost::asio::error::eof || SSL_SHORT_READ(e))
            XDEBUG_WITH_ID << "EOF in socket, stop.";
        else
            XERROR_WITH_ID << "Read error: " << e.message();
        // TODO enhance
        connection_.stop();
        return;
    }

    XDEBUG_WITH_ID << "Read, length: " << length << ", content:\n"
                   << std::string(data, length);

    if (length <= 0) {
        XWARN_WITH_ID << "Read, no data.";
        // TODO enhance
        connection_.stop();
        return;
    }

    if (parser_->MessageCompleted()) {
        XERROR_WITH_ID << "Read, message is already completed.";
        // TODO enhance
        connection_.stop();
        return;
    }

    auto consumed = parser_->consume(data, length);
    if (consumed <= 0) {
        XERROR_WITH_ID << "Read, message parsing failure.";
        // TODO enhance
        connection_.stop();
        return;
    }

    if (!parser_->MessageCompleted())
        connection_.read();
}

void ClientAdapter::onWrite(const boost::system::error_code& e) {
    if (e) {
        XERROR_WITH_ID << "Write error: " << e.message();
        connection_.stop();
        return;
    }

    if (https_ && !ssl_built_) {
        XDEBUG_WITH_ID << "SSL reply written.";
        auto ca = ResourceManager::GetCertManager().GetCertificate(remote_host_);
        auto dh = ResourceManager::GetCertManager().GetDHParameters();
        connection_.handshake(ca, dh);
        return;
    }

    XDEBUG_WITH_ID << "Response data written.";
}

void ClientAdapter::onHeadersComplete(message::http::HttpMessage&) {
    // do nothing here currently
}

void ClientAdapter::onBody(message::http::HttpMessage&) {
    // do nothing here currently
}

void ClientAdapter::onMessageComplete(message::http::HttpMessage& message) {
    if (!https_) { // a normal HTTP request or a CONNECT request
        if (!ParseRemotePeer()) {
            XERROR_WITH_ID << "Remote host/port parsing failure.";
            // TODO enhance
            connection_.stop();
            return;
        }
    }

    // a CONNECT request, and the SSL has not been setup
    if (https_ && !ssl_built_) {
        const static std::string ssl_response("HTTP/1.1 200 Connection Established\r\n"
                                              "Content-Length: 0\r\n"
                                              "Connection: keep-alive\r\n"
                                              "Proxy-Connection: keep-alive\r\n\r\n");
        connection_.write(ssl_response);
        return;
    }

    auto bridge = connection_.bridgeConnection();
    if (!bridge) {
        XERROR_WITH_ID << "No bridge connection.";
        // TODO enhance
        connection_.stop();
        return;
    }

    bridge->write(*message_);
}

bool ClientAdapter::ParseRemotePeer() {
    if (!parser_->HeadersCompleted()) {
        XERROR_WITH_ID << "Incomplete header.";
        return false;
    }

    auto method = message_->getField(message::http::HttpMessage::kRequestMethod);
    if (method.length() == 7 && method[0] == 'C' && method[1] == 'O') {
        remote_host_ = message_->getField(message::http::HttpMessage::kRequestUri);
        remote_port_ = 443;

        auto sep = host_.find(':');
        if (sep != std::string::npos) {
            remote_port_ = remote_host_.substr(sep + 1);
            remote_host_ = remote_host_.substr(0, sep);
        }
    } else {
        remote_port_ = 80;
        if (!message_->findHeader("Host", remote_host_)) {
            XERROR_WITH_ID << "Header \"Host\" not found.";
            return false;
        }

        auto sep = remote_host_.find(':');
        if (sep != std::string::npos) {
            remote_port_ = remote_host_.substr(sep + 1);
            remote_host_ = remote_host_.substr(0, sep);
        }
    }

    return true;
}

} // namespace net
} // namespace xproxy
