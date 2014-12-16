#ifndef SERVER_CONNECTION_HPP
#define SERVER_CONNECTION_HPP

#include "x/net/connection.hpp"

namespace x {
namespace net {

class server_connection : public connection {
public:
    server_connection(context_ptr ctx);

    DEFAULT_DTOR(server_connection);

    virtual void start();

    virtual void connect();

    virtual void handshake(ssl::certificate ca = ssl::certificate(), DH *dh = nullptr);

    virtual void on_connect();

    virtual void on_read();

    virtual void on_write();

    virtual void on_handshake();

private:
    boost::asio::ip::tcp::resolver resolver_;
};

} // namespace net
} // namespace x

#endif // SERVER_CONNECTION_HPP