#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include "connection.h"
#include "counter.h"

class ClientConnection : public Connection, public Counter<ClientConnection> {
public:
    ClientConnection(std::shared_ptr<Session> session);
    virtual ~ClientConnection() = default;

    virtual void OnBodyComplete();

    virtual void init();

    virtual void OnMessageExchangeComplete();

private:
    virtual void connect() {} // do nothing here

    virtual void OnRead(const boost::system::error_code& e, std::size_t);

    virtual void OnWritten(const boost::system::error_code& e, std::size_t length);

    virtual void OnTimeout(const boost::system::error_code& e);
};

#endif // CLIENT_CONNECTION_H
