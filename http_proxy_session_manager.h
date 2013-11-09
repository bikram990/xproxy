#ifndef HTTP_PROXY_SESSION_MANAGER_H
#define HTTP_PROXY_SESSION_MANAGER_H

#include "http_proxy_session.h"
#include "singleton.h"

class HttpProxySessionManager : private boost::noncopyable {
    friend class Singleton<HttpProxySessionManager>;
public:
    static void Start(HttpProxySession::Ptr session);
    static void Stop(HttpProxySession::Ptr session);
    static void StopAll();

    static HttpProxySessionManager& instance();

private:
    DECLARE_GENERAL_CONSTRUCTOR(HttpProxySessionManager)
    DECLARE_GENERAL_DESTRUCTOR(HttpProxySessionManager)

    std::set<HttpProxySession::Ptr> sessions_;
};

#endif // HTTP_PROXY_SESSION_MANAGER_H
