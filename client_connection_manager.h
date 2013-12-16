#ifndef CLIENT_CONNECTION_MANAGER_H
#define CLIENT_CONNECTION_MANAGER_H

#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

class Connection;
class ProxyServer;

class ClientConnectionManager : private boost::noncopyable {
    friend class ProxyServer;
public:
    void start(boost::shared_ptr<Connection> connection);
    void stop(boost::shared_ptr<Connection> connection);
    boost::shared_ptr<Connection> find(const std::string& host, short port);
    void StopAll();

private:
    // the key is host:port
    typedef std::map<std::string, boost::shared_ptr<Connection>> container_type;

    ClientConnectionManager() {}

    container_type connections_;
};

#endif // CLIENT_CONNECTION_MANAGER_H
