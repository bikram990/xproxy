#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <set>
#include "connection.h"

class ProxyServer;

class ConnectionManager : private boost::noncopyable {
    friend class ProxyServer;
public:
    void start(ConnectionPtr connection);
    void stop(ConnectionPtr connection);
    void StopAll();

private:
    ConnectionManager() {}

    std::set<ConnectionPtr> connections_;
};

#endif // CONNECTION_MANAGER_H
