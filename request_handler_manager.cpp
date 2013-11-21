#include "http_request.h"
#include "request_handler_manager.h"
#include "resource_manager.h"
#include "proxy_server.h"

void RequestHandlerManager::AsyncHandleRequest(HttpProxySession::Ptr session) {
    HttpRequest *request = session->Request();
    if(!request) {
        // TODO change the error code
        session->OnResponseReceived(boost::asio::error::invalid_argument);
        return;
    }

    bool proxy = ResourceManager::GetRuleConfig().RequestProxy(request->host());
    if(proxy) {
        boost::lock_guard<boost::mutex> lock(proxy_mutex_);
        std::vector<RequestHandler::Ptr>::iterator it = std::find_if(proxy_handlers_.begin(),
                                                                     proxy_handlers_.end(),
                                                                     AvailabilitySearcher());
        if(it != proxy_handlers_.end()) {
            XDEBUG << "Cached proxy handler is found.";
            (*it)->Update(session);
            (*it)->AsyncHandleRequest();
            return;
        } else {
            RequestHandler::Ptr h(new ProxyHandler(session));
            proxy_handlers_.push_back(h);
            h->AsyncHandleRequest();
            return;
        }
    } else {
        boost::lock_guard<boost::mutex> lock(direct_mutex_);
        std::vector<RequestHandler::Ptr>::iterator it = std::find_if(direct_handlers_.begin(),
                                                                     direct_handlers_.end(),
                                                                     AvailabilitySearcher());
        if(it != direct_handlers_.end()) {
            XDEBUG << "Cached direct handler is found.";
            (*it)->Update(session);
            (*it)->AsyncHandleRequest();
            return;
        } else {
            RequestHandler::Ptr h(new DirectHandler(session));
            direct_handlers_.push_back(h);
            h->AsyncHandleRequest();
            return;
        }
    }
}
