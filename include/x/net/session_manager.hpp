#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include "x/net/session.hpp"

namespace x {
namespace net {

class session_manager {
public:
    DEFAULT_CTOR(session_manager);
    DEFAULT_DTOR(session_manager);

    void start(const session_ptr& session) {
        sessions_.insert(session);
        session->start();
    }

    void erase(const session_ptr& session) {
        sessions_.erase(session);
    }

    void stop_all() {
        std::for_each(sessions_.begin(), sessions_.end(),
                      [](const session_ptr& session) {
            #warning the code below will change the container!!!
            session->stop();
        });
    }

private:
    std::set<session_ptr> sessions_;

    MAKE_NONCOPYABLE(session_manager);
};

} // namespace net
} // namespace x

#endif // SESSION_MANAGER_HPP
