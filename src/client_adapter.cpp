#include "xproxy/log/log.hpp"
#include "xproxy/message/http/http_request.hpp"
#include "xproxy/net/client_adapter.hpp"

namespace xproxy {
namespace net {

ClientAdapter::ClientAdapter(Connection& connection)
    : connection_(connection),
      message_(new message::http::HttpRequest),
      parser_(new message::http::HttpParser(*message_, HTTP_REQUEST, this)) {}

void ClientAdapter::onConnect(const boost::system::error_code& e) {
    // do nothing here, a client connection is always connected
    XERROR << "Should NEVER be called.";
}

void ClientAdapter::onHandshake(const boost::system::error_code& e) {
    XDEBUG_ID_WITH(connection_) << "=> onHandshake()";
    if (e) {
        XERROR_ID_WITH(connection_) << "Handshake error: " << e.message();
        // TODO enhance
        connection_.stop();
        return;
    }

    connection_.context()->client_ssl_setup = true;
    message_->reset();
    parser_->reset();
    connection_.read();
    XDEBUG_ID_WITH(connection_) << "<= onHandshake()";
}

void ClientAdapter::onRead(const boost::system::error_code &e, const char *data, std::size_t length) {
    XDEBUG_ID_WITH(connection_) << "=> onRead()";

    if (!connection_.socket().is_open()) {
        XDEBUG_ID_WITH(connection_) << "Socket closed.";
        return;
    }

    if (connection_.timerRunning()) {
        XDEBUG_ID_WITH(connection_) << "Cancel running timer.";
        connection_.cancelTimer();
    }

    if (e) {
        if (e == boost::asio::error::eof || SSL_SHORT_READ(e)) {
            XDEBUG_ID_WITH(connection_) << "EOF in socket, stop.";
            connection_.setConnected(false);
        } else
            XERROR_ID_WITH(connection_) << "Read error: " << e.message();
        // TODO enhance
        connection_.stop();
        return;
    }

    if (length <= 0) {
        XWARN_ID_WITH(connection_) << "Read, no data.";
        // TODO enhance
        connection_.stop();
        return;
    }

    XDEBUG_ID_WITH(connection_) << "Read, length: " << length;

    if (parser_->messageCompleted()) {
        XERROR_ID_WITH(connection_) << "Read, message is already completed.";
        // TODO enhance
        connection_.stop();
        return;
    }

    auto consumed = parser_->consume(data, length);
    if (consumed <= 0) {
        XERROR_ID_WITH(connection_) << "Read, message parsing failure.";
        // TODO enhance
        connection_.stop();
        return;
    }

    if (!parser_->messageCompleted())
        connection_.read();

    XDEBUG_ID_WITH(connection_) << "<= onRead()";
}

void ClientAdapter::onWrite(const boost::system::error_code& e) {
    XDEBUG_ID_WITH(connection_) << "=> onWrite()";
    if (e) {
        XERROR_ID_WITH(connection_) << "Write error: " << e.message();
        connection_.stop();
        return;
    }

    auto context = connection_.context();
    if (context->https && !context->client_ssl_setup) {
        XDEBUG_ID_WITH(connection_) << "SSL reply written.";
        assert(!context->remote_host.empty());
        auto ca = ssl::CertificateManager::instance().getCertificate(context->remote_host);
        auto dh = ssl::CertificateManager::instance().getDHParameters();
        connection_.handshake(ca, dh);
        return;
    }

    XDEBUG_ID_WITH(connection_) << "Response data written.";

    if (context->message_exchange_completed) {
        if (parser_->keepAlive()) {
            context->reset();
            parser_->reset();
            message_->reset();
            connection_.startTimer(kDefaultClientTimeout);
            connection_.read();
        } else {
            XDEBUG_ID_WITH(connection_) << "No keep-alive, closing.";
            // TODO enhance
            connection_.stop();
        }
    }

    XDEBUG_ID_WITH(connection_) << "<= onWrite()";
}

void ClientAdapter::onTimeout(const boost::system::error_code&) {
    XDEBUG_ID_WITH(connection_) << "Client connection timed out, stop.";
    // TODO enhance
    connection_.stop();
}

void ClientAdapter::onHeadersComplete(message::http::HttpMessage&) {
    // do nothing here currently
    XDEBUG_ID_WITH(connection_) << "onHeadersComplete(), no action.";
}

void ClientAdapter::onBody(message::http::HttpMessage&) {
    // do nothing here currently
    XDEBUG_ID_WITH(connection_) << "onBody(), no action.";
}

void ClientAdapter::onMessageComplete(message::http::HttpMessage& message) {
    XDEBUG_ID_WITH(connection_) << "=> onMessageComplete()";
    auto context = connection_.context();
    if (!context->https) { // a normal HTTP request or a CONNECT request
        if (!ParseRemotePeer(context->remote_host, context->remote_port)) {
            XERROR_ID_WITH(connection_) << "Remote host/port parsing failure.";
            // TODO enhance
            connection_.stop();
            return;
        }
    }

    // a CONNECT request, and the SSL has not been setup
    if (context->https && !context->client_ssl_setup) {
        XDEBUG_ID_WITH(connection_) << "SSL CONNECT request.";
        const static std::string ssl_response("HTTP/1.1 200 Connection Established\r\n"
                                              "Content-Length: 0\r\n"
                                              "Connection: keep-alive\r\n"
                                              "Proxy-Connection: keep-alive\r\n\r\n");
        connection_.write(ssl_response);
        return;
    }

    auto bridge = connection_.getBridgeConnection();
    if (!bridge) {
        XERROR_ID_WITH(connection_) << "No bridge connection.";
        // TODO enhance
        connection_.stop();
        return;
    }

    if (bridge->timerRunning()) {
        XDEBUG_ID_WITH(connection_) << "Cancel bridge's running timer.";
        bridge->cancelTimer();
    }

    std::shared_ptr<memory::ByteBuffer> buf(new memory::ByteBuffer);
    if (message.serialize(*buf) > 0)
        bridge->write(buf);

    XDEBUG_ID_WITH(connection_) << "<= onMessageComplete()";
}

bool ClientAdapter::ParseRemotePeer(std::string& remote_host, std::string& remote_port) {
    if (!parser_->headersCompleted()) {
        XERROR_ID_WITH(connection_) << "Incomplete header.";
        return false;
    }

    auto method = message_->getField(message::http::HttpMessage::kRequestMethod);
    if (method.length() == 7 && method[0] == 'C' && method[1] == 'O') {
        connection_.context()->https = true;
        remote_host = message_->getField(message::http::HttpMessage::kRequestUri);
        remote_port = "443";

        auto sep = remote_host.find(':');
        if (sep != std::string::npos) {
            remote_port = remote_host.substr(sep + 1);
            remote_host = remote_host.substr(0, sep);
        }
    } else {
        remote_port = "80";
        if (!message_->findHeader("Host", remote_host)) {
            XERROR_ID_WITH(connection_) << "Header \"Host\" not found.";
            return false;
        }

        auto sep = remote_host.find(':');
        if (sep != std::string::npos) {
            remote_port = remote_host.substr(sep + 1);
            remote_host = remote_host.substr(0, sep);
        }
    }

    return true;
}

} // namespace net
} // namespace xproxy
