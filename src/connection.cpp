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

void Connection::stop() {
    XDEBUG_WITH_ID << "Stopping conneciton...";
    // 1. close socket
    closeSocket();

    // 2. close socket of bridge connection
    if (bridge_connection_)
        bridge_connection_->closeSocket();

    // 3. remove the reference-to-each-other
    bridge_connection_->setBridgeConnection(nullptr);
    bridge_connection_.reset();

    // 4. remove reference in manager
    if (manager_)
        manager_->erase(shared_from_this());
}

void Connection::read() {
    XDEBUG_WITH_ID << "=> read()";
    auto self(shared_from_this());
    boost::asio::async_read(*socket_,
                            boost::asio::buffer(buffer_in_),
                            boost::asio::transfer_at_least(1),
                            [self, this] (const boost::system::error_code& e, std::size_t length) {
        if (adapter_)
            adapter_->onRead(e, buffer_in_.data(), length);
    });
    XDEBUG_WITH_ID << "<= read()";
}

void Connection::write(const message::Message &message) {
    XDEBUG_WITH_ID << "=> write(message)";
    std::shared_ptr<memory::ByteBuffer> buf(new memory::ByteBuffer);
    auto size = message.serialize(*buf);
    if (size <= 0) {
        XERROR_WITH_ID << "Write, no content.";
        return;
    }

    buffer_out_.push_back(buf);
    doWrite();
    XDEBUG_WITH_ID << "=> write(message)";
}

void Connection::write(const std::string& str) {
    XDEBUG_WITH_ID << "=> write(string)";
    std::shared_ptr<memory::ByteBuffer> buf(new memory::ByteBuffer(str.length()));
    *buf << str;
    buffer_out_.push_back(buf);

    doWrite();
    XDEBUG_WITH_ID << "<= write(string)";
}

Connection::Connection(boost::asio::io_service& service,
                       ConnectionAdapter *adapter,
                       SharedConnectionContext context,
                       ConnectionManager *manager)
    : service_(service),
      socket_(SocketFacade::create(service)),
      adapter_(adapter),
      context_(context),
      manager_(manager),
      writing_(false) {}

void Connection::doWrite() {
    if (writing_)
        return;

    XDEBUG_WITH_ID << "=> doWrite()";
    writing_ = true;
    auto& candidate = buffer_out_.front();
    auto self(shared_from_this());
    socket_->async_write_some(boost::asio::buffer(candidate->data(), candidate->size()),
                              [self, candidate, this] (const boost::system::error_code& e, std::size_t length) {
        // 1. reset the writing_ flag to false after written
        writing_ = false;

        // 2. if there is error, go to adapter to handle the error
        if (e) {
            XDEBUG_WITH_ID << "Write error, pass it to adapter.";
            if (adapter_)
                adapter_->onWrite(e);
            return;
        }

        // 3. no error, we check if all data in the buffer has been written
        if (length < candidate->size()) {
            XWARN_WITH_ID << "Write incomplete, continue.";
            candidate->erase(0, length);
            doWrite();
            return;
        }

        // 4. remove the written buffer, and check if there are more buffers are added
        buffer_out_.erase(buffer_out_.begin());
        if (!buffer_out_.empty()) {
            XDEBUG_WITH_ID << "More buffers added, continue writing.";
            doWrite();
            return;
        }

        // 5. all data has been written, go to adapter
        if (adapter_)
            adapter_->onWrite(e);
    });
    XDEBUG_WITH_ID << "<= doWrite()";
}

void ConnectionManager::start(ConnectionPtr& connection) {
    connections_.insert(connection);
    connection->start();
}

void ConnectionManager::erase(ConnectionPtr& connection) {
    auto it = std::find(connections_.begin(), connections_.end(), connection);
    if (it == connections_.end()) {
        XWARN << "Connection not found, id: " << connection->id();
        return;
    }

    connections_.erase(connection);
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
    context_->local_host = socket_->socket().remote_endpoint().address().to_string();
    context_->local_port = std::to_string(socket_->socket().remote_endpoint().port());
    read();
}

void ClientConnection::connect(const std::string&, const std::string&) {
    XERROR_WITH_ID << "Should NEVER be called.";
}

void ClientConnection::handshake(ResourceManager::CertManager::CAPtr ca, ResourceManager::CertManager::DHParametersPtr dh) {
    XDEBUG_WITH_ID << "=> handshake()";
    auto self(shared_from_this());
    socket_->useSSL([self, this] (const boost::system::error_code& e) {
        if (adapter_)
            adapter_->onHandshake(e);
    }, SocketFacade::kServer, ca, dh);
    XDEBUG_WITH_ID << "<= handshake()";
}

ClientConnection::ClientConnection(boost::asio::io_service& service,
                                   SharedConnectionContext context,
                                   ConnectionManager *manager)
    : Connection(service, new ClientAdapter(*this), context, manager) {}

void ServerConnection::start() {
    XDEBUG_WITH_ID << "Starting server connection...";
    assert(!context_->remote_host.empty());
    assert(!context_->remote_port.empty());

    connect(context_->remote_host, context_->remote_port);
}

void ServerConnection::connect(const std::string& host, const std::string& port) {
    XDEBUG_WITH_ID << "=> connect()";
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
    XDEBUG_WITH_ID << "<= connect()";
}

void ServerConnection::handshake(ResourceManager::CertManager::CAPtr ca, ResourceManager::CertManager::DHParametersPtr dh) {
    XDEBUG_WITH_ID << "=> handshake()";
    auto self(shared_from_this());
    socket_->useSSL<ResourceManager::CertManager::CAPtr,
            ResourceManager::CertManager::DHParametersPtr>([self, this] (const boost::system::error_code& e) {
        if (adapter_)
            adapter_->onHandshake(e);
    });
    XDEBUG_WITH_ID << "<= handshake()";
}

ServerConnection::ServerConnection(boost::asio::io_service& service,
                                   SharedConnectionContext context,
                                   ConnectionManager *manager)
    : Connection(service, new ServerAdapter(*this), context, manager), resolver_(service) {}

ConnectionPtr createBridgedConnections(boost::asio::io_service& service,
                                       ConnectionManager *client_manager,
                                       ConnectionManager *server_manager) {
    SharedConnectionContext context(new ConnectionContext);
    ConnectionPtr client(new ClientConnection(service, context, client_manager));
    ConnectionPtr server(new ServerConnection(service, context, server_manager));
    client->setBridgeConnection(server);
    server->setBridgeConnection(client);

    return client;
}

} // namespace net
} // namespace xproxy
