#ifndef HTTP_PROXY_SESSION_MANAGER_H
#define HTTP_PROXY_SESSION_MANAGER_H

#include <set>
#include "http_proxy_session.h"

class ProxyServer;

class HttpProxySessionManager : private boost::noncopyable {
    friend class ProxyServer;
public:
    void Start(HttpProxySession::Ptr session);
    void Stop(HttpProxySession::Ptr session);
    void StopAll();

    DECLARE_GENERAL_DESTRUCTOR(HttpProxySessionManager)

private:
    DECLARE_GENERAL_CONSTRUCTOR(HttpProxySessionManager)

    std::set<HttpProxySession::Ptr> sessions_;
};

#endif // HTTP_PROXY_SESSION_MANAGER_H
