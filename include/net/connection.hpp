#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <array>
#include <list>
#include <memory>
#include <set>
#include <vector>
#include <boost/asio.hpp>
#include "common.hpp"

namespace xproxy {
namespace memory { class ByteBuffer; }
namespace message { class Message; }
namespace net {

class SocketFacade;
class Connection;
typedef std::shared_ptr<Connection> ConnectionPtr;

class ConnectionAdapter {
public:
    virtual void onConnect(const boost::system::error_code& e) = 0;
    virtual void onHandshake(const boost::system::error_code& e) = 0;
    virtual void onRead(const boost::system::error_code& e, const char *data, std::size_t length) = 0;
    virtual void onWrite(const boost::system::error_code& e, xproxy::memory::ByteBuffer& buffer, std::size_t length) = 0;
};

class Connection : public std::enable_shared_from_this<Connection> {
public:
    // typedef std::vector<char> buffer_type;
    typedef xproxy::memory::ByteBuffer buffer_type;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void connect(const std::string& host, const std::string& port) = 0;
    virtual void handshake() = 0;
    virtual void read() = 0;
    virtual void write(const xproxy::message::Message& message) = 0;

protected:
    virtual void doWrite() = 0;
    virtual void doConnect() = 0;

private:
    enum { kBufferSize = 8192 };

    boost::asio::io_service& service_;
    std::unique_ptr<SocketFacade> socket_;

    std::array<char, kBufferSize> buffer_in_;
    std::list<buffer_type> buffer_out_;

    std::unique_ptr<ConnectionAdapter> adapter_;
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
