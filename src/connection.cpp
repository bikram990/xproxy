#include "xproxy/log/log.hpp"
#include "xproxy/message/http/http_message.hpp"
#include "xproxy/net/client_adapter.hpp"
#include "xproxy/net/connection.hpp"
#include "xproxy/net/server_adapter.hpp"
#include "xproxy/net/socket_facade.hpp"
#include "xproxy/plugin/plugin_manager.hpp"

namespace xproxy {
namespace net {

void Connection::closeSocket() {
    if (socket_ && connected_)
        socket_->close();
}

void Connection::startTimer(long timeout) {
    XDEBUG_WITH_ID << "=> startTimer()";
    auto self(shared_from_this());
    timer_.start(timeout, [self, this] (const boost::system::error_code& e) {
        if (adapter_)
            adapter_->onTimeout(e);
    });
    XDEBUG_WITH_ID << "<= startTimer()";
}

void Connection::stop() {
    if (stopped_) {
        XDEBUG_WITH_ID << "Connection is already stopped.";
        return;
    }

    XDEBUG_WITH_ID << "Stopping connection...";
    // 1. cancel timer
    if (timer_.running())
        timer_.cancel();

    // 2. cancel bridge connection's timer
    if (bridge_connection_ && bridge_connection_->timer_.running())
        bridge_connection_->timer_.cancel();

    // 3. close socket
    if (connected_) {
        closeSocket();
        connected_ = false;
    }

    // 4. close socket of bridge connection
    if (bridge_connection_ && bridge_connection_->connected()) {
        bridge_connection_->closeSocket();
        bridge_connection_->setConnected(false);
    }

    // 5. remove the reference-to-each-other
    if (bridge_connection_) {
        bridge_connection_->setBridgeConnection(nullptr);
        bridge_connection_.reset();
    }

    // 6. remove reference in manager
    if (manager_)
        manager_->erase(shared_from_this());

    // 7. set the stopped_ flag to true
    stopped_ = true;
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

void Connection::write(const std::shared_ptr<memory::ByteBuffer>& buffer) {
    XDEBUG_WITH_ID << "=> write(buffer)";
    if (!buffer || buffer->size() <= 0) {
        XDEBUG_WITH_ID << "Write, no content.";
        return;
    }

    buffer_out_.push_back(buffer);
    doWrite();
    XDEBUG_WITH_ID << "<= write(buffer)";
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
                       SharedConnectionContext context,
                       ConnectionManager *manager)
    : connected_(false),
      stopped_(false),
      socket_(SocketFacade::create(service)),
      context_(context),
      manager_(manager),
      service_(service),
      timer_(service),
      writing_(false) {}

void Connection::doWrite() {
    if (writing_) {
        XDEBUG_WITH_ID << "Connection is writing.";
        return;
    }

    if (buffer_out_.empty()) {
        XDEBUG_WITH_ID << "Empty out buffer.";
        return;
    }

    if (!beforeWrite()) {
        XDEBUG_WITH_ID << "beforeWrite() hook returns false.";
        return;
    }

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
            XDEBUG_WITH_ID << "Write incomplete, continue.";
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

void ConnectionManager::erase(const ConnectionPtr& connection) {
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
        // Note: we have to set the manager_ to null here, as in
        // function stop(), it will call manager_.erase(...) to
        // remove the connection if manager_ is not null, erase
        // an element in looping a STL container without change
        // the iterator? Yeah, the program will crash
        connection->setConnectionManager(nullptr);
        connection->stop();
    });
    connections_.clear();
}

void ClientConnection::start() {
    XDEBUG_WITH_ID << "Starting client connection...";
    connected_ = true;
    context_->local_host = socket_->socket().remote_endpoint().address().to_string();
    context_->local_port = std::to_string(socket_->socket().remote_endpoint().port());
    read();
}

void ClientConnection::connect(const std::string&, const std::string&) {
    XERROR_WITH_ID << "Should NEVER be called.";
}

void ClientConnection::handshake(ssl::Certificate ca, DH *dh) {
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
    : Connection(service, context, manager) {}

bool ClientConnection::beforeWrite() {
    return true; // we don't need any preparation work here
}

void ClientConnection::initAdapter(plugin::PluginChain *chain) {
    if (!adapter_)
        adapter_.reset(new ClientAdapter(*this, chain));
}

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

void ServerConnection::handshake(ssl::Certificate, DH*) {
    XDEBUG_WITH_ID << "=> handshake()";
    auto self(shared_from_this());
    socket_->useSSL([self, this] (const boost::system::error_code& e) {
        if (adapter_)
            adapter_->onHandshake(e);
    });
    XDEBUG_WITH_ID << "<= handshake()";
}

ServerConnection::ServerConnection(boost::asio::io_service& service,
                                   SharedConnectionContext context,
                                   ConnectionManager *manager)
    : Connection(service, context, manager), resolver_(service) {}

bool ServerConnection::beforeWrite() {
    if (connected_)
        return true;
    start();
    return false;
}

void ServerConnection::initAdapter(plugin::PluginChain *chain) {
    if (!adapter_)
        adapter_.reset(new ServerAdapter(*this, chain));
}

ConnectionPtr createBridgedConnections(boost::asio::io_service& service,
                                       ConnectionManager *client_manager,
                                       ConnectionManager *server_manager) {
    SharedConnectionContext context(new ConnectionContext);
    ConnectionPtr client(new ClientConnection(service, context, client_manager));
    ConnectionPtr server(new ServerConnection(service, context, server_manager));
    plugin::PluginChain *cpc = nullptr;
    plugin::PluginChain *spc = nullptr;
    plugin::PluginChain::create(&cpc, &spc);
    client->initAdapter(cpc);
    server->initAdapter(spc);
    client->setBridgeConnection(server);
    server->setBridgeConnection(client);

    XDEBUG << "Connections bridged, client[id: " << client->id()
           << "] <=> server[id: " << server->id() << "].";

    return client;
}

} // namespace net
} // namespace xproxy
