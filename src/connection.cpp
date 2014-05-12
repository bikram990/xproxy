#include "net/connection.hpp"

namespace xproxy {
namespace net {

void ConnectionManager::start(ConnectionPtr &connection) {
    connections_.insert(connection);
    connection->start();
}

void ConnectionManager::stop(ConnectionPtr &connection) {
    auto it = std::find(connections_.begin(), connections_.end(), connection);
    if (it == connections_.end()) {
        XWARN << "connection not found.";
        return;
    }

    connections_.erase(connection);
    connection->stop();
}

void ConnectionManager::stopAll() {
    std::for_each(connections_.begin(), connections_.end(),
                  [](ConnectionPtr& connection) {
        connection->stop();
    });
    connections_.clear();
}

} // namespace net
} // namespace xproxy
