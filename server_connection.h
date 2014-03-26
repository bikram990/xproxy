#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include "connection.h"
#include "counter.h"

class ServerConnection : public Connection, public Counter<ServerConnection> {
public:
    ServerConnection(std::shared_ptr<Session> session);
    virtual ~ServerConnection();

    virtual void connect();

    virtual void OnRead(const boost::system::error_code& e);

    virtual void OnWritten(const boost::system::error_code& e);

    virtual void OnTimeout(const boost::system::error_code& e);

    virtual void OnConnected(const boost::system::error_code& e);

    virtual void NewDataCallback(std::shared_ptr<Session> session);

    virtual void CompleteCallback(std::shared_ptr<Session> session);

    void host(const std::string& host) { host_ = host; }

    void port(unsigned short port) { port_ = port; }

private:
    std::string host_;
    unsigned short port_;
    boost::asio::ip::tcp::resolver resolver_;
};

#endif // SERVER_CONNECTION_H
