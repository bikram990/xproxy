#include "log/log.hpp"
#include "net/server_adapter.hpp"
#include "message/http/http_response.hpp"
#include "util/timer.hpp"

namespace xproxy {
namespace net {

ServerAdapter::ServerAdapter(Connection& connection)
    : connection_(connection),
      timer_(connection.service()),
      message_(new message::http::HttpResponse),
      parser_(new message::http::HttpParser(*message_, HTTP_RESPONSE, this)),
      connected_(false), https_(false) {}

void ServerAdapter::onConnect(const boost::system::error_code& e) {
    if (e) {
        XERROR_ID_WITH(connection_) << "Connect error: " << e.message();
        // TODO enhance
        connection_.stop();
        return;
    }

    if (https_) {
        connection_.handshake();
        return;
    }
}

void ServerAdapter::onHandshake(const boost::system::error_code& e) {
    if (e) {
        XERROR_ID_WITH(connection_) << "Handshake error: " << e.message();
        // TODO enhance
        connection_.stop();
        return;
    }

    connection_.doWrite();
}

void ServerAdapter::onRead(const boost::system::error_code& e, const char *data, std::size_t length) {
    if (e) {
        if (e == boost::asio::error::eof || SSL_SHORT_READ(e)) {
            XDEBUG_ID_WITH(connection_) << "EOF in socket.";
            connected_ = false;
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

    if (!parser_->messageCompleted())
        connection_.read();
}

void ServerAdapter::onWrite(const boost::system::error_code& e) {
    if (e) {
        XERROR_ID_WITH(connection_) << "Write error: " << e.message();
        // TODO enhance
        connection_.stop();
        return;
    }

    connection_.read();
}

void ServerAdapter::onHeadersComplete(message::http::HttpMessage& message) {
    // do nothing here currently
}

void ServerAdapter::onBody(message::http::HttpMessage& message) {
    auto bridge = connection_.getBridgeConnection();
    if (!bridge) {
        XERROR_ID_WITH(connection_) << "No bridge connection.";
        // TODO enhance
        connection_.stop();
        return;
    }

    bridge->write(message);
}

void ServerAdapter::onMessageComplete(message::http::HttpMessage& message) {
    onBody(message);

    // TODO do cleanup job here
}

} // namespace net
} // namespace xproxy
