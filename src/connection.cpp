#include "xproxy/log/log.hpp"
#include "xproxy/message/http/http_message.hpp"
#include "xproxy/net/client_adapter.hpp"
#include "xproxy/net/connection.hpp"
#include "xproxy/net/server_adapter.hpp"
#include "xproxy/net/socket_facade.hpp"

namespace xproxy {
namespace net {

void Connection::closeSocket() {
    if (socket_)
        socket_->close();
}

void Connection::read() {
    auto self(shared_from_this());
    boost::asio::async_read(*socket_,
                            boost::asio::buffer(buffer_in_),
                            boost::asio::transfer_at_least(1),
                            [self, this] (const boost::system::error_code& e, std::size_t length) {
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

Connection::Connection(boost::asio::io_service& service, SharedConnectionContext context, ConnectionAdapter *adapter)
    : service_(service), socket_(SocketFacade::create(service)), adapter_(adapter), context_(context) {}

void Connection::doWrite() {
    auto& candidate = buffer_out_.front();
    auto buf = boost::asio::buffer(candidate->data(), candidate->size());
    auto self(shared_from_this());
    socket_->async_write_some(buf,
                              [self, candidate, this] (const boost::system::error_code& e, std::size_t length) {
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
                  [](const ConnectionPtr& connection) {
        connection->stop();
    });
    connections_.clear();
}

void ClientConnection::start() {
    XDEBUG_WITH_ID << "Starting client connection...";
    // TODO add local_host, local_port in ConnectionContext, and set them here
    read();
}

void ClientConnection::stop() {
    XDEBUG_WITH_ID << "Stopping client conneciton...";
    socket_->close();
    // remove the reference-to-each-other here
    bridge_connection_->setBridgeConnection(nullptr);
    bridge_connection_.reset();
}

void ClientConnection::connect(const std::string& host, const std::string& port) {
    // do nothing here, a client connection is always connected
}

void ClientConnection::handshake(ResourceManager::CertManager::CAPtr ca, ResourceManager::CertManager::DHParametersPtr dh) {
    auto self(shared_from_this());
    socket_->useSSL([self, this] (const boost::system::error_code& e) {
        if (adapter_)
            adapter_->onHandshake(e);
    }, SocketFacade::kServer, ca, dh);
}

void ClientConnection::doConnect() {
    // do nothing here
}

ClientConnection::ClientConnection(boost::asio::io_service& service, SharedConnectionContext context)
    : Connection(service, context, new ClientAdapter(*this)) {}

void ServerConnection::start() {
    XDEBUG_WITH_ID << "Starting server connection...";
    connect(context_->remote_host, context_->remote_port);
}

void ServerConnection::stop() {
    XDEBUG_WITH_ID << "Stopping server connection...";
    socket_->close();
    // remove the reference-to-each-other here
    bridge_connection_->setBridgeConnection(nullptr);
    bridge_connection_.reset();
}

void ServerConnection::connect(const std::string& host, const std::string& port) {
    using namespace boost::asio::ip;

    auto self(shared_from_this());
    resolver_.async_resolve(tcp::resolver::query(host, port),
                            [self, this, host, port] (const boost::system::error_code& e, tcp::resolver::iterator it) {
        if (e) {
            XERROR_WITH_ID << "Resolve error: " << e.message();
            // TODO enhance
            stop();
            return;
        }

        XDEBUG_WITH_ID << "Resolved, connecting, host: " << host
                       << ", port: " << port
                       << ", address: " << it->endpoint().address();

        socket_->async_connect(it, [self, this] (const boost::system::error_code& e, tcp::resolver::iterator) {
            if (adapter_)
                adapter_->onConnect(e);
        });
    });
}

void ServerConnection::handshake(ResourceManager::CertManager::CAPtr ca, ResourceManager::CertManager::DHParametersPtr dh) {
    auto self(shared_from_this());
    socket_->useSSL<ResourceManager::CertManager::CAPtr,
            ResourceManager::CertManager::DHParametersPtr>([self, this] (const boost::system::error_code& e) {
        if (adapter_)
            adapter_->onHandshake(e);
    });
}

void ServerConnection::doConnect() {
    // TODO do nothing here, however, we need to re-design the remote connecting mechanism
}

ServerConnection::ServerConnection(boost::asio::io_service& service, SharedConnectionContext context)
    : Connection(service, context, new ServerAdapter(*this)), resolver_(service) {}

ConnectionPtr createBridgedConnections(boost::asio::io_service& service) {
    SharedConnectionContext context(new ConnectionContext);
    ConnectionPtr client(new ClientConnection(service, context));
    ConnectionPtr server(new ServerConnection(service, context));
    client->setBridgeConnection(server);
    server->setBridgeConnection(client);

    return client;
}

} // namespace net
} // namespace xproxy
