#include <boost/bind.hpp>
#include "client_connection_manager.h"
#include "connection.h"
#include "log.h"

void ClientConnectionManager::start(ConnectionPtr connection) {
    connections_.insert(connection);
    connection->start();
}

void ClientConnectionManager::stop(ConnectionPtr connection) {
    connections_.erase(connection);
}

void ClientConnectionManager::StopAll() {
    std::for_each(connections_.begin(), connections_.end(),
                  boost::bind(&Connection::stop, _1));
    connections_.clear();
}
