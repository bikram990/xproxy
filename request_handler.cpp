#include "http_direct_handler.h"
#include "http_proxy_handler.h"
#include "https_direct_handler.h"
#include "https_proxy_handler.h"
#include "request_handler.h"
#include "resource_manager.h"

RequestHandler::RequestHandler() {
    TRACE_THIS_PTR;
}

RequestHandler::~RequestHandler() {
    TRACE_THIS_PTR;
}

RequestHandler *RequestHandler::CreateHandler(HttpProxySession& session,
                                              HttpRequestPtr request) {
    // TODO may add socks handler here
    bool proxy = ResourceManager::instance().GetRuleConfig().RequestProxy(request->host());
    RequestHandler *handler = NULL;
    if(request->method() == "CONNECT")
        if(proxy)
            handler = new HttpsProxyHandler(session, request);
        else
            handler = new HttpsDirectHandler(session, request);
    else
        if(proxy)
            handler = new HttpProxyHandler(session, request);
        else
            handler = new HttpDirectHandler(session, request);
    return handler;
}
