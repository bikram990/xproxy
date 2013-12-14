#include <boost/bind.hpp>
#include "connection_manager.h"
#include "log.h"

void ConnectionManager::start(ConnectionPtr connection) {
    connections_.insert(session);
    connection->start();
}

void ConnectionManager::stop(ConnectionPtr connection) {
    connections_.erase(session);
}

void ConnectionManager::StopAll() {
    std::for_each(sessions_.begin(), sessions_.end(),
                  boost::bind(&Connection::stop, _1));
}
