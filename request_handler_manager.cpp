#include "http_request.h"
#include "request_handler_manager.h"
#include "resource_manager.h"

namespace {
    static Singleton<RequestHandlerManager> manager_;
}

inline RequestHandlerManager& RequestHandlerManager::instance() {
    return manager_.get();
}

void RequestHandlerManager::AsyncHandleRequest(HttpProxySession::Ptr session) {
    HttpRequest *request = session->Request();
    if(!request) {
        // TODO change the error code
        session->OnResponseReceived(boost::asio::error::invalid_argument);
        return;
    }

    bool proxy = ResourceManager::instance().GetRuleConfig().RequestProxy(request->host());
    if(proxy) {
        boost::lock_guard<boost::mutex> lock(instance().proxy_mutex_);
        std::vector<RequestHandler::Ptr>::iterator it = std::find_if(instance().proxy_handlers_.begin(),
                                                                     instance().proxy_handlers_.end(),
                                                                     AvailabilitySearcher());
        if(it != instance().proxy_handlers_.end()) {
            XDEBUG << "Cached proxy handler is found.";
            (*it)->Update(session);
            (*it)->AsyncHandleRequest();
            return;
        } else {
            RequestHandler::Ptr h(new ProxyHandler(session));
            instance().proxy_handlers_.push_back(h);
            h->AsyncHandleRequest();
            return;
        }
    } else {
        boost::lock_guard<boost::mutex> lock(instance().direct_mutex_);
        std::vector<RequestHandler::Ptr>::iterator it = std::find_if(instance().direct_handlers_.begin(),
                                                                     instance().direct_handlers_.end(),
                                                                     AvailabilitySearcher());
        if(it != instance().direct_handlers_.end()) {
            XDEBUG << "Cached direct handler is found.";
            (*it)->Update(session);
            (*it)->AsyncHandleRequest();
            return;
        } else {
            RequestHandler::Ptr h(new DirectHandler(session));
            instance().direct_handlers_.push_back(h);
            h->AsyncHandleRequest();
            return;
        }
    }
}
