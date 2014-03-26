#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include "connection.h"
#include "counter.h"

class ServerConnection : public Connection, public Counter<ServerConnection> {
public:
    ServerConnection();
    virtual ~ServerConnection();

    virtual void InitMessage();

    virtual void OnRead(const boost::system::error_code& e);

    virtual void OnWritten(const boost::system::error_code& e);

    virtual void OnTimeout(const boost::system::error_code& e);

    virtual void NewDataCallback(std::shared_ptr<Session> session);

    virtual void CompleteCallback(std::shared_ptr<Session> session);
};

#endif // SERVER_CONNECTION_H
