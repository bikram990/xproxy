#include "server_connection_manager.h"

ConnectionPtr *ServerConnectionManager::RequireConnection(const std::string &host, short port) {
    boost::lock_guard<boost::mutex> lock(lock_);

    if(connections_.size() > 0) {
        ConnectionPtr connection = connections_.front();
        connections_.pop_front();
        return connection;
    }

    return ConnectionPtr(new ServerConnection(fetch_service_));
}

void ServerConnectionManager::ReleaseConnection(ConnectionPtr connection) {
    if(!connection)
        return;

    boost::lock_guard<boost::mutex> lock(lock_);
    connections_.push_front(connection);
}
