#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include "connection.h"
#include "counter.h"

class ClientConnection : public Connection, public Counter<ClientConnection> {
public:
    ClientConnection();
    virtual ~ClientConnection();

    virtual void InitMessage();

    virtual void OnRead(const boost::system::error_code& e);

    virtual void OnWritten(const boost::system::error_code& e);
};

#endif // CLIENT_CONNECTION_H
