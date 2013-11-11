#ifndef REQUEST_HANDLER_MANAGER_H
#define REQUEST_HANDLER_MANAGER_H

#include "http_proxy_session.h"
#include "request_handler.h"
#include "singleton.h"

class RequestHandlerManager : private boost::noncopyable {
    friend class Singleton<RequestHandlerManager>;
public:
    static void AsyncHandleRequest(HttpProxySession::Ptr session);
private:
    static RequestHandlerManager& instance();

    struct AvailabilitySearcher {
        bool operator()(const RequestHandler::Ptr& handler) {
            return handler->Available();
        }
    };

    DECLARE_GENERAL_CONSTRUCTOR(RequestHandlerManager)
    DECLARE_GENERAL_DESTRUCTOR(RequestHandlerManager)

    std::vector<RequestHandler::Ptr> direct_handlers_;
    std::vector<RequestHandler::Ptr> proxy_handlers_;

    boost::mutex direct_mutex_;
    boost::mutex proxy_mutex_;
};

#endif // REQUEST_HANDLER_MANAGER_H
