#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <set>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

class ProxyServer;
class Session;

class SessionManager : private boost::noncopyable {
    friend class ProxyServer;
public:
    void start(boost::shared_ptr<Session> session);
    void stop(boost::shared_ptr<Session> session);
    void StopAll();

private:
    SessionManager() {}

    std::set<boost::shared_ptr<Session>> sessions_;
};

#endif // SESSION_MANAGER_H
