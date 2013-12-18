#ifndef SERVER_CONNECTION_MANAGER_H
#define SERVER_CONNECTION_MANAGER_H

#include <list>
#include <map>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

class Connection;
class ProxyServer;

class ServerConnectionManager {
    friend class ProxyServer;
public:
    boost::shared_ptr<Connection> RequireConnection(const std::string &host, unsigned short port);

    void ReleaseConnection(boost::shared_ptr<Connection> connection);

private:
    typedef std::list<boost::shared_ptr<Connection>> value_type;
    typedef std::map<std::string, value_type> cache_type;

    ServerConnectionManager(boost::asio::io_service& service)
        : fetch_service_(service) {}

    boost::asio::io_service& fetch_service_;
    cache_type connections_;
    boost::mutex lock_;
};

#endif // SERVER_CONNECTION_MANAGER_H
