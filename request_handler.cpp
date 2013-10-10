#include "log.h"
#include "http_direct_handler.h"
#include "http_proxy_handler.h"
#include "https_direct_handler.h"
#include "https_proxy_handler.h"
#include "http_proxy_session.h"
#include "request_handler.h"

RequestHandler::RequestHandler() {
    TRACE_THIS_PTR;
}

RequestHandler::~RequestHandler() {
    TRACE_THIS_PTR;
}

RequestHandler *RequestHandler::CreateHandler(HttpProxySession& session,
                                              HttpRequestPtr request) {
    //if(data[0] == 'C' && data[1] == 'O' && data[2] == 'N')
    //    return NULL;//nullptr; // TODO this is https connection, implement it later
    // TODO may add socks handler here
    //else
    if(request->method() == "CONNECT")
        return new HttpsDirectHandler(session, request);
    else
        return CreateHttpHandler(session, request);
}

RequestHandler *RequestHandler::CreateHttpHandler(HttpProxySession& session,
                                                  HttpRequestPtr request) {
    // TODO add logic here after HttpProxyHandler is implemented
    //return new HttpDirectHandler(session, request);
    //return new HttpProxyHandler(session, request);
    return new HttpsProxyHandler(session, request);
}
