#include "log.h"
#include "message/http/http_message.hpp"
#include "net/connection.hpp"

namespace xproxy {
namespace net {

void Connection::closeSocket() {
    if (socket_)
        socket_->close();
}

void Connection::read() {
    auto self(shared_from_this());
    boost::asio::async_read(*socket,
                            buffer_in_,
                            boost::asio::transfer_at_least(1),
                            [self, this](const boost::system::error_code& e, std::size_t length) {
        if (adapter_)
            adapter_->onRead(e, buffer_in_.data(), length);
    });
}

void Connection::write(const message::Message &message) {
    std::shared_ptr<memory::ByteBuffer> buf(new memory::ByteBuffer);
    auto size = message.serialize(*buf);
    if (size <= 0) {
        XERROR_WITH_ID << "Write, no content.";
        return;
    }

    buffer_out_.push_back(buf);

    doWrite();
}

void Connection::write(const std::string& str) {
    std::shared_ptr<memory::ByteBuffer> buf(new memory::ByteBuffer);
    *buf << str;
    buffer_out_.push_back(buf);

    doWrite();
}

void Connection::doWrite() {
    auto& candidate = buf_out_.front();
    auto buf = boost::asio::buffer(candidate->data(), candidate->size());
    auto self(shared_from_this());
    socket_->async_write_some(buf,
                              [self, this](const boost::system::error_code& e, std::size_t length) {
        if (length < candidate->size()) {
            XWARN_WITH_ID << "Write incomplete, continue.";
            candidate->erase(0, length);
            doWrite();
            return;
        }

        buffer_out_.erase(buffer_out_.begin());
        if (!buffer_out_.empty()) {
            XDEBUG_WITH_ID << "More buffers added, continue writing.";
            doWrite();
            return;
        }

        if (adapter_)
            adapter_->onWrite(e);
    });
}

void ConnectionManager::start(ConnectionPtr &connection) {
    connections_.insert(connection);
    connection->start();
}

void ConnectionManager::stop(ConnectionPtr &connection) {
    auto it = std::find(connections_.begin(), connections_.end(), connection);
    if (it == connections_.end()) {
        XWARN << "connection not found.";
        return;
    }

    connections_.erase(connection);
    connection->stop();
}

void ConnectionManager::stopAll() {
    std::for_each(connections_.begin(), connections_.end(),
                  [](ConnectionPtr& connection) {
        connection->stop();
    });
    connections_.clear();
}

} // namespace net
} // namespace xproxy
