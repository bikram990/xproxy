#include "session.h"
#include "session_manager.h"

void SessionManager::start(boost::shared_ptr<Session> session) {
    sessions_.insert(session);
    session->start();
}

void SessionManager::stop(boost::shared_ptr<Session> session) {
    sessions_.erase(session);
    session->stop();
}

void SessionManager::StopAll() {
    std::for_each(sessions_.begin(), sessions_.end(),
                  boost::bind(&Session::stop, _1));
    sessions_.clear();
}
