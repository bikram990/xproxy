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

private:
    virtual void connect() {} // do nothing here

    virtual void OnRead(const boost::system::error_code& e);

    virtual void OnWritten(const boost::system::error_code& e);

    virtual void OnTimeout(const boost::system::error_code& e);
};

#endif // CLIENT_CONNECTION_H
