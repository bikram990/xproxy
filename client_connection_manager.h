#ifndef CLIENT_CONNECTION_MANAGER_H
#define CLIENT_CONNECTION_MANAGER_H

#include <set>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

class Connection;
class ProxyServer;

class ClientConnectionManager : private boost::noncopyable {
    friend class ProxyServer;
public:
    void start(boost::shared_ptr<Connection> connection);
    void stop(boost::shared_ptr<Connection> connection);
    void StopAll();

private:
    ClientConnectionManager() {}

    std::set<boost::shared_ptr<Connection>> connections_;
};

#endif // CLIENT_CONNECTION_MANAGER_H
