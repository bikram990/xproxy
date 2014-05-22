#include "xproxy/log/log.hpp"
#include "xproxy/message/http/http_response.hpp"
#include "xproxy/net/server_adapter.hpp"

namespace xproxy {
namespace net {

ServerAdapter::ServerAdapter(Connection& connection)
    : connection_(connection),
      timer_(connection.service()),
      message_(new message::http::HttpResponse),
      parser_(new message::http::HttpParser(*message_, HTTP_RESPONSE, this)) {}

void ServerAdapter::onConnect(const boost::system::error_code& e) {
    XDEBUG_ID_WITH(connection_) << "=> onConnect()";
    if (e) {
        XERROR_ID_WITH(connection_) << "Connect error: " << e.message();
        // TODO enhance
        connection_.stop();
        return;
    }

    auto context = connection_.context();
    context->server_connected = true;

    if (context->https && !context->server_ssl_setup) {
        XDEBUG_ID_WITH(connection_) << "SSL, handshaking...";
        connection_.handshake();
        return;
    }

    connection_.doWrite();
    XDEBUG_ID_WITH(connection_) << "<= onConnect()";
}

void ServerAdapter::onHandshake(const boost::system::error_code& e) {
    XDEBUG_ID_WITH(connection_) << "=> onHandshake()";
    if (e) {
        XERROR_ID_WITH(connection_) << "Handshake error: " << e.message();
        // TODO enhance
        connection_.stop();
        return;
    }

    connection_.context()->server_ssl_setup = true;
    connection_.doWrite();
    XDEBUG_ID_WITH(connection_) << "<= onHandshake()";
}

void ServerAdapter::onRead(const boost::system::error_code& e, const char *data, std::size_t length) {
    XDEBUG_ID_WITH(connection_) << "=> onRead()";
    if (e) {
        if (e == boost::asio::error::eof || SSL_SHORT_READ(e)) {
            XDEBUG_ID_WITH(connection_) << "EOF in socket.";
            connection_.context()->server_connected = false;
            connection_.closeSocket();
        } else {
            XERROR_ID_WITH(connection_) << "Read error: " << e.message();
            // TODO enhance
            connection_.stop();
            return;
        }
    }

    if(length <= 0) {
        XWARN_ID_WITH(connection_) << "Read, no data.";
        // TODO enhance
        connection_.stop();
        return;
    }

    XDEBUG_ID_WITH(connection_) << "Read, length: " << length << ", content:\n"
                   << std::string(data, length);

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

    if (!parser_->messageCompleted()) {
        connection_.read();
    } else {
        if (parser_->keepAlive()) {
            parser_->reset();
            message_->reset();
            timer_.start(kDefaultServerTimeout, [this] (const boost::system::error_code&) {
                XDEBUG_ID_WITH(connection_) << "Server connection timed out, close.";
                auto context = connection_.context();
                context->server_connected = false;
                if (context->https)
                    context->server_ssl_setup = false;
                connection_.closeSocket();
                // TODO do we need to create a new socket to replace the old one?
                // can we use the old one, and reconnect it?
            });
        } else {
            XDEBUG_ID_WITH(connection_) << "No keep-alive, closing.";
            parser_->reset();
            message_->reset();
            auto context = connection_.context();
            context->server_connected = false;
            if (context->https)
                context->server_ssl_setup = false;
            connection_.closeSocket();
            // TODO do we need to create a new socket to replace the old one?
            // can we use the old one, and reconnect it?
        }
    }

    XDEBUG_ID_WITH(connection_) << "<= onRead()";
}

void ServerAdapter::onWrite(const boost::system::error_code& e) {
    XDEBUG_ID_WITH(connection_) << "=> onWrite()";
    if (e) {
        XERROR_ID_WITH(connection_) << "Write error: " << e.message();
        // TODO enhance
        connection_.stop();
        return;
    }

    connection_.read();
    XDEBUG_ID_WITH(connection_) << "<= onWrite()";
}

void ServerAdapter::onHeadersComplete(message::http::HttpMessage&) {
    // do nothing here currently
    XDEBUG_ID_WITH(connection_) << "onHeadersComplete(), no action.";
}

void ServerAdapter::onBody(message::http::HttpMessage& message) {
    XDEBUG_ID_WITH(connection_) << "=> onBody()";
    auto bridge = connection_.getBridgeConnection();
    if (!bridge) {
        XERROR_ID_WITH(connection_) << "No bridge connection.";
        // TODO enhance
        connection_.stop();
        return;
    }

    bridge->write(message);
    XDEBUG_ID_WITH(connection_) << "<= onBody()";
}

void ServerAdapter::onMessageComplete(message::http::HttpMessage& message) {
    XDEBUG_ID_WITH(connection_) << "=> onMessageComplete()";
    // TODO: if the client's onWrite() is called before this, it will lead to
    // unwanted behavior, consider to define a method like
    // onMessageExchangeComplete() to call
    connection_.context()->message_exchange_completed = true;
    onBody(message);
    XDEBUG_ID_WITH(connection_) << "<= onMessageComplete()";
}

} // namespace net
} // namespace xproxy
