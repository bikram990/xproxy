#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <set>
#include "common.h"

class ProxyServer;
class Session;

class SessionManager {
    friend class ProxyServer;
public:
    void start(std::shared_ptr<Session> session);
    void stop(std::shared_ptr<Session> session);
    void StopAll();

private:
    SessionManager() {}

    std::set<std::shared_ptr<Session>> sessions_;

private:
    DISABLE_COPY_AND_ASSIGNMENT(SessionManager);
};

#endif // SESSION_MANAGER_H
