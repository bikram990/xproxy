#ifndef SERVER_CONNECTION_MANAGER_H
#define SERVER_CONNECTION_MANAGER_H

#include <list>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

class Connection;
class ProxyServer;

class ServerConnectionManager {
    friend class ProxyServer;
public:
    boost::shared_ptr<Connection> RequireConnection(const std::string &host, short port);

    void ReleaseConnection(boost::shared_ptr<Connection> connection);

private:
    ServerConnectionManager(boost::asio::io_service& service)
        : fetch_service_(service) {}

    boost::asio::io_service& fetch_service_;
    std::list<boost::shared_ptr<Connection>> connections_;
    boost::mutex lock_;
};

#endif // SERVER_CONNECTION_MANAGER_H
