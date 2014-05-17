#include "log.h"
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
        XERROR_WITH_ID << "Connect error: " << e.message();
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
        XERROR_WITH_ID << "Handshake error: " << e.message();
        // TODO enhance
        connection_.stop();
        return;
    }

    connection_.doWrite();
}

void ServerAdapter::onRead(const boost::system::error_code& e, const char *data, std::size_t length) {
    if (e) {
        if (e == boost::asio::error::eof || SSL_SHORT_READ(e)) {
            XDEBUG_WITH_ID << "EOF in socket.";
            connected_ = false;
            connection_.socket_->close();
        } else {
            XERROR_WITH_ID << "Read error: " << e.message();
            // TODO enhance
            connection_.stop();
            return;
        }
    }

    if(length <= 0) {
        XWARN_WITH_ID << "Read, no data.";
        // TODO enhance
        connection_.stop();
        return;
    }

    XDEBUG_WITH_ID << "Read, length: " << length << ", content:\n"
                   << std::string(data, length);

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

void ServerAdapter::onWrite(const boost::system::error_code& e) {
    if (e) {
        XERROR_WITH_ID << "Write error: " << e.message();
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
    auto bridge = connection_.bridgeConnection();
    if (!bridge) {
        XERROR_WITH_ID << "No bridge connection.";
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
