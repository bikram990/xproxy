#ifndef CONNECTION_MANAGER_HPP
#define CONNECTION_MANAGER_HPP

#include "x/net/connection.hpp"

namespace x {
namespace net {

class connection_manager {
public:
    DEFAULT_CTOR_AND_DTOR(connection_manager);

    void add(connection_ptr conn) {
        connections_.insert(conn);
    }

    void erase(connection_ptr conn) {
        auto it = std::find(connections_.begin(), connections_.end(), conn);
        if (it == connections_.end()) {
            XWARN << "connection [id: " << conn->id() << "] not found.";
            return;
        }

        connections_.erase(it);
    }

    void stop_all() {
        std::for_each(connections_.begin(), connections_.end(),
                      [] (connection_ptr conn) {
            conn->detach();
            conn->stop();
        });

        connections_.clear();
    }

private:
    std::set<connection_ptr> connections_;
};

} // namespace net
} // namespace x

#endif // CONNECTION_MANAGER_HPP

