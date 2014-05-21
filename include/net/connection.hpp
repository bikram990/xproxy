#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <array>
#include <list>
#include <memory>
#include <set>
#include <vector>
#include <boost/asio.hpp>
#include "common.hpp"
#include "net/socket_facade.hpp"
#include "resource_manager.h"
#include "util/counter.hpp"

namespace xproxy {
namespace memory { class ByteBuffer; }
namespace message { class Message; }
namespace net {

class Connection;
typedef std::shared_ptr<Connection> ConnectionPtr;

class ConnectionAdapter {
public:
    virtual void onConnect(const boost::system::error_code& e) = 0;
    virtual void onHandshake(const boost::system::error_code& e) = 0;
    virtual void onRead(const boost::system::error_code& e, const char *data, std::size_t length) = 0;
    virtual void onWrite(const boost::system::error_code& e) = 0;
};

struct ConnectionContext {
    ConnectionContext() : https(false), proxied(false) {}

    bool https;
    bool proxied;
    std::string remote_host;
    std::string remote_port;
};

typedef std::shared_ptr<ConnectionContext> SharedConnectionContext;

ConnectionPtr createBridgedConnections(boost::asio::io_service& service);

class Connection : public util::Counter<Connection>, public std::enable_shared_from_this<Connection> {
public:
    // typedef std::vector<char> buffer_type;
    typedef xproxy::memory::ByteBuffer buffer_type;

    boost::asio::io_service& service() const { return service_; }

    SocketFacade::socket_type& socket() const { return socket_->socket(); }

    SharedConnectionContext context() const { return context_; }

    ConnectionPtr getBridgeConnection() const { return bridge_connection_; }

    void setBridgeConnection(ConnectionPtr connection) { bridge_connection_ = connection; }

    void closeSocket();

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void connect(const std::string& host, const std::string& port) = 0;
    virtual void handshake(ResourceManager::CertManager::CAPtr ca = nullptr,
                           ResourceManager::CertManager::DHParametersPtr dh = nullptr) = 0;
    virtual void read();
    virtual void write(const xproxy::message::Message& message);
    virtual void write(const std::string& str);
    virtual void doWrite();

protected:
    Connection(boost::asio::io_service& service,
               SharedConnectionContext context,
               ConnectionAdapter *adapter);
    DEFAULT_VIRTUAL_DTOR(Connection);

protected:
    virtual void doConnect() = 0;

protected:
    std::unique_ptr<SocketFacade> socket_;
    std::unique_ptr<ConnectionAdapter> adapter_;
    ConnectionPtr bridge_connection_;
    SharedConnectionContext context_;

private:
    enum { kBufferSize = 8192 };

    boost::asio::io_service& service_;

    std::array<char, kBufferSize> buffer_in_;
    std::list<std::shared_ptr<buffer_type>> buffer_out_;
};

class ClientConnection : public Connection {
    friend ConnectionPtr createBridgedConnections(boost::asio::io_service& service);
public:
    virtual void start();
    virtual void stop();
    virtual void connect(const std::string& host, const std::string& port);
    virtual void handshake(ResourceManager::CertManager::CAPtr ca, ResourceManager::CertManager::DHParametersPtr dh);

    DEFAULT_VIRTUAL_DTOR(ClientConnection);

protected:
    virtual void doConnect();

protected:
    ClientConnection(boost::asio::io_service& service, SharedConnectionContext context);
};

class ServerConnection : public Connection {
    friend ConnectionPtr createBridgedConnections(boost::asio::io_service& service);
public:
    virtual void start();
    virtual void stop();
    virtual void connect(const std::string& host, const std::string& port);
    virtual void handshake(ResourceManager::CertManager::CAPtr ca, ResourceManager::CertManager::DHParametersPtr dh);

    DEFAULT_VIRTUAL_DTOR(ServerConnection);

protected:
    virtual void doConnect();

protected:
    ServerConnection(boost::asio::io_service& service, SharedConnectionContext context);

private:
    boost::asio::ip::tcp::resolver resolver_;
};

class ConnectionManager {
public:
    void start(ConnectionPtr& connection);
    void stop(ConnectionPtr& connection);
    void stopAll();

    DEFAULT_CTOR_AND_DTOR(ConnectionManager);

private:
    std::set<ConnectionPtr> connections_;

private:
    MAKE_NONCOPYABLE(ConnectionManager);
};

} // namespace net
} // namespace xproxy

#endif // CONNECTION_HPP
