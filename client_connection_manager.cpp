#include <boost/bind.hpp>
#include "client_connection_manager.h"
#include "log.h"

void ClientConnectionManager::start(ConnectionPtr connection) {
    connections_.insert(session);
    connection->start();
}

void ClientConnectionManager::stop(ConnectionPtr connection) {
    connections_.erase(session);
}

void ClientConnectionManager::StopAll() {
    std::for_each(sessions_.begin(), sessions_.end(),
                  boost::bind(&Connection::stop, _1));
}
