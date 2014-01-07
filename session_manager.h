#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <boost/noncopyable.hpp>

class ProxyServer;

class SessionManager : private boost::noncopyable {
    friend class ProxyServer;
private:
    SessionManager() {}
};

#endif // SESSION_MANAGER_H
