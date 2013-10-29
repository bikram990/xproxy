#ifndef HTTP_PROXY_SESSION_MANAGER_H
#define HTTP_PROXY_SESSION_MANAGER_H

#include <set>
#include <boost/noncopyable.hpp>
#include "http_proxy_session.h"


class HttpProxySessionManager : private boost::noncopyable {
public:
    HttpProxySessionManager();
    ~HttpProxySessionManager();

    void Start(HttpProxySession::Ptr session);
    void Stop(HttpProxySession::Ptr session);
    void StopAll();

private:
    std::set<HttpProxySession::Ptr> sessions_;
};

#endif // HTTP_PROXY_SESSION_MANAGER_H
