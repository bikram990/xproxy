#include <boost/bind.hpp>
#include "http_proxy_session_manager.h"
#include "log.h"

HttpProxySessionManager::HttpProxySessionManager() {
    TRACE_THIS_PTR;
}

HttpProxySessionManager::~HttpProxySessionManager() {
    // do nothing here
    // TODO should we clean sockets in dispatcher_ here?
    TRACE_THIS_PTR;
}

void HttpProxySessionManager::Start(HttpProxySession::Ptr session) {
    sessions_.insert(session);
    session->Start();
}

void HttpProxySessionManager::Stop(HttpProxySession::Ptr session) {
    sessions_.erase(session);
    session->Stop();
}

void HttpProxySessionManager::StopAll() {
    std::for_each(sessions_.begin(), sessions_.end(),
                  boost::bind(&HttpProxySession::Stop, _1));
    sessions_.clear();
}
