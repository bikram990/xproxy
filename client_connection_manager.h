#ifndef CLIENT_CONNECTION_MANAGER_H
#define CLIENT_CONNECTION_MANAGER_H

#include <set>
#include "connection.h"

class ProxyServer;

class ClientConnectionManager : private boost::noncopyable {
    friend class ProxyServer;
public:
    void start(ConnectionPtr connection);
    void stop(ConnectionPtr connection);
    void StopAll();

private:
    ClientConnectionManager() {}

    std::set<ConnectionPtr> connections_;
};

#endif // CLIENT_CONNECTION_MANAGER_H
