#ifndef SERVER_CONNECTION_MANAGER_H
#define SERVER_CONNECTION_MANAGER_H

#include <list>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include "server_connection.h"

class ProxyServer;

class ServerConnectionManager {
    friend class ProxyServer;
public:
    ConnectionPtr *RequireConnection(const std::string &host, short port);

    void ReleaseConnection(ConnectionPtr connection);

private:
    ServerConnectionManager(boost::asio::io_service& service)
        : fetch_service_(service) {}

    boost::asio::io_service& fetch_service_;
    std::list<ConnectionPtr> connections_;
    boost::mutex lock_;
};

#endif // SERVER_CONNECTION_MANAGER_H
