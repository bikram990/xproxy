#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <list>
#include <boost/asio.hpp>
#include "xproxy/common.hpp"
#include "xproxy/net/socket_facade.hpp"
#include "resource_manager.h"
#include "xproxy/util/counter.hpp"
#include "xproxy/util/timer.hpp"

namespace xproxy {
namespace memory { class ByteBuffer; }
namespace message { class Message; }
namespace net {

class Connection;
class ConnectionManager;
typedef std::shared_ptr<Connection> ConnectionPtr;

class ConnectionAdapter {
public:
    virtual void onConnect(const boost::system::error_code& e) = 0;
    virtual void onHandshake(const boost::system::error_code& e) = 0;
    virtual void onRead(const boost::system::error_code& e, const char *data, std::size_t length) = 0;
    virtual void onWrite(const boost::system::error_code& e) = 0;
    virtual void onTimeout(const boost::system::error_code& e) = 0;
};

struct ConnectionContext {
    ConnectionContext()
        : https(false),
          proxied(false),
          client_ssl_setup(false),
          server_ssl_setup(false),
          message_exchange_completed(false) {}

    bool https;
    bool proxied;
    bool client_ssl_setup;
    bool server_ssl_setup;
    bool message_exchange_completed;
    std::string local_host;
    std::string local_port;
    std::string remote_host;
    std::string remote_port;
};

typedef std::shared_ptr<ConnectionContext> SharedConnectionContext;

ConnectionPtr createBridgedConnections(
    boost::asio::io_service& service,
    ConnectionManager *client_manager,
    ConnectionManager *server_manager
);

class Connection : public util::Counter<Connection>, public std::enable_shared_from_this<Connection> {
public:
    // typedef std::vector<char> buffer_type;
    typedef xproxy::memory::ByteBuffer buffer_type;

    boost::asio::io_service& service() const { return service_; }

    SocketFacade::socket_type& socket() const { return socket_->socket(); }

    bool connected() const { return connected_; }

    void setConnected(bool connected) { connected_ = connected; }

    SharedConnectionContext context() const { return context_; }

    ConnectionPtr getBridgeConnection() const { return bridge_connection_; }

    void setBridgeConnection(ConnectionPtr connection) { bridge_connection_ = connection; }

    void closeSocket();

    void startTimer(long timeout);

    virtual void initAdapter() = 0;

    virtual void start() = 0;
    virtual void stop();

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
               ConnectionManager *manager);
    DEFAULT_VIRTUAL_DTOR(Connection);

    virtual bool beforeWrite() = 0;

protected:
    bool connected_;
    std::unique_ptr<SocketFacade> socket_;
    std::unique_ptr<ConnectionAdapter> adapter_;
    SharedConnectionContext context_;
    ConnectionPtr bridge_connection_;
    ConnectionManager *manager_;

private:
    enum { kBufferSize = 8192 };

    boost::asio::io_service& service_;

    util::Timer timer_;

    std::array<char, kBufferSize> buffer_in_;
    std::list<std::shared_ptr<buffer_type>> buffer_out_;

    bool writing_;
};

class ClientConnection : public Connection {
    friend ConnectionPtr createBridgedConnections(boost::asio::io_service& service,
                                                  ConnectionManager *client_manager,
                                                  ConnectionManager *server_manager);
public:
    virtual void start();
    virtual void connect(const std::string& host, const std::string& port);
    virtual void handshake(ResourceManager::CertManager::CAPtr ca, ResourceManager::CertManager::DHParametersPtr dh);

    virtual void initAdapter();

    // DEFAULT_VIRTUAL_DTOR(ClientConnection);
    virtual ~ClientConnection() { XDEBUG_WITH_ID << "~ClientConnection"; }

protected:
    ClientConnection(boost::asio::io_service& service,
                     SharedConnectionContext context,
                     ConnectionManager *manager);

    virtual bool beforeWrite();
};

class ServerConnection : public Connection {
    friend ConnectionPtr createBridgedConnections(boost::asio::io_service& service,
                                                  ConnectionManager *client_manager,
                                                  ConnectionManager *server_manager);
public:
    virtual void start();
    virtual void connect(const std::string& host, const std::string& port);
    virtual void handshake(ResourceManager::CertManager::CAPtr ca, ResourceManager::CertManager::DHParametersPtr dh);

    virtual void initAdapter();

    // DEFAULT_VIRTUAL_DTOR(ServerConnection);
    virtual ~ServerConnection() { XDEBUG_WITH_ID << "~ServerConnection"; }

protected:
    ServerConnection(boost::asio::io_service& service,
                     SharedConnectionContext context,
                     ConnectionManager *manager);

    virtual bool beforeWrite();

private:
    boost::asio::ip::tcp::resolver resolver_;
};

class ConnectionManager {
public:
    void start(ConnectionPtr& connection);
    void erase(const ConnectionPtr& connection);
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
