#include <boost/bind.hpp>
#include "http_proxy_session_manager.h"
#include "log.h"

namespace {
    static Singleton<HttpProxySessionManager> manager_;
}

inline HttpProxySessionManager& HttpProxySessionManager::instance() {
    return manager_.get();
}

void HttpProxySessionManager::Start(HttpProxySession::Ptr session) {
    instance().sessions_.insert(session);
    session->Start();
}

void HttpProxySessionManager::Stop(HttpProxySession::Ptr session) {
    instance().sessions_.erase(session);
    session->Stop();
}

void HttpProxySessionManager::StopAll() {
    std::for_each(instance().sessions_.begin(), instance().sessions_.end(),
                  boost::bind(&HttpProxySession::Stop, _1));
    instance().sessions_.clear();
}
