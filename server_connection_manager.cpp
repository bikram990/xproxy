#include "server_connection.h"
#include "server_connection_manager.h"

ConnectionPtr ServerConnectionManager::RequireConnection(const std::string &host, short port) {
    std::string key(host + ":" + std::to_string(port));

    boost::lock_guard<boost::mutex> lock(lock_);

    auto it = connections_.find(key);
    if(it == connections_.end()) {
        ConnectionPtr connection(new ServerConnection(fetch_service_));
        connection->SetRemoteAddress(host, port);
        return connection;
    }

    value_type& v = it->second;
    if(v.size() > 0) {
        ConnectionPtr connection = v.front();
        v.pop_front();
        return connection;
    }

    ConnectionPtr connection(new ServerConnection(fetch_service_));
    connection->SetRemoteAddress(host, port);
    return connection;
}

void ServerConnectionManager::ReleaseConnection(ConnectionPtr connection) {
    if(!connection)
        return;

    std::string key(connection->host() + ":" + std::to_string(connection->port()));

    boost::lock_guard<boost::mutex> lock(lock_);
    auto it = connections_.find(key);
    if(it != connections_.end()) {
        it->second.push_front(connection);
        return;
    }

    value_type v;
    v.push_front(connection);
    connections_.insert(std::pair<std::string, value_type>(key, v));
}
