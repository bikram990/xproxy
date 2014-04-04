#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include "connection.h"
#include "counter.h"

class ServerConnection : public Connection, public Counter<ServerConnection> {
public:
    ServerConnection(std::shared_ptr<Session> session);
    virtual ~ServerConnection() = default;

    virtual void OnBody();
    virtual void OnBodyComplete();

    virtual void init();

    virtual void OnMessageExchangeComplete();

private:
    enum {
        kSocketTimeout = 15,
        kBufferSize = 8192
    };

    virtual void connect();

    virtual void OnRead(const boost::system::error_code& e, std::size_t);

    virtual void OnWritten(const boost::system::error_code& e, std::size_t length);

    virtual void OnTimeout(const boost::system::error_code& e);

    virtual void OnConnected(const boost::system::error_code& e);

private:
    boost::asio::ip::tcp::resolver resolver_;
};

#endif // SERVER_CONNECTION_H
