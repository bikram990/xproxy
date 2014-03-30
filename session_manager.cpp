#include "session.h"
#include "session_manager.h"

void SessionManager::start(std::shared_ptr<Session> session) {
    sessions_.insert(session);
    session->start();
}

void SessionManager::stop(std::shared_ptr<Session> session) {
    sessions_.erase(session);
    session->stop();
}

void SessionManager::StopAll() {
    std::for_each(sessions_.begin(), sessions_.end(),
                  [](std::shared_ptr<Session> session) {
                      session->stop();
                  });
    sessions_.clear();
}
