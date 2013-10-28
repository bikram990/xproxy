#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include "http_request.h"
#include "http_response.h"


class HttpClient;
class HttpProxySession;

class RequestHandler : public boost::enable_shared_from_this<RequestHandler> {
public:
    typedef boost::function<void(const boost::system::error_code&)> handler_type;

    static RequestHandler *CreateHandler(HttpProxySession& session,
                                         HttpRequest& request,
                                         HttpResponse& response,
                                         handler_type handler);
    virtual void AsyncHandleRequest();
    virtual ~RequestHandler();

protected:
    RequestHandler(HttpProxySession& session,
                   HttpRequest& request,
                   HttpResponse& response,
                   handler_type handler);

    virtual HttpRequest *WrapRequest() = 0;
    virtual void init() = 0;

    HttpProxySession& session_;
    HttpRequest& request_;
    HttpResponse& response_;

    handler_type handler_;
    boost::asio::ssl::context *remote_ssl_context_;
    HttpClient *client_;

    bool inited_;
};

class DirectHandler : public RequestHandler {
    friend class RequestHandler;
public:
    virtual HttpRequest *WrapRequest();
private:
    DirectHandler(HttpProxySession& session,
                  HttpRequest& request,
                   HttpResponse& response,
                   handler_type handler)
        : RequestHandler(session, request, response, handler) { init(); }
    virtual void init();
};

class ProxyHandler : public RequestHandler {
    friend class RequestHandler;
public:
    virtual HttpRequest *WrapRequest();

    virtual ~ProxyHandler() {
        if(proxy_request_) {
            delete proxy_request_;
            proxy_request_ = NULL;
        }
    }
private:
    ProxyHandler(HttpProxySession& session,
                 HttpRequest& request,
                   HttpResponse& response,
                   handler_type handler)
        : RequestHandler(session, request, response, handler), proxy_request_(NULL) { init(); }

    virtual void init();
    void BuildProxyRequest();

    HttpRequest *proxy_request_;
};

#endif // REQUEST_HANDLER_H
