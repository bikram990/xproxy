#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include "connection.h"
#include "counter.h"

class ClientConnection : public Connection, public Counter<ClientConnection> {
public:
    ClientConnection(std::shared_ptr<Session> session);
    virtual ~ClientConnection();

    virtual void InitMessage();

    virtual void OnRead(const boost::system::error_code& e);

    virtual void OnWritten(const boost::system::error_code& e);

    virtual void OnTimeout(const boost::system::error_code& e);

    virtual void NewDataCallback(std::shared_ptr<Session> session);

    virtual void CompleteCallback(std::shared_ptr<Session> session);
};

#endif // CLIENT_CONNECTION_H
