#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <boost/asio/ssl.hpp>
#include "http_proxy_session.h"

class HttpClient;
class HttpRequest;
class HttpResponse;

class RequestHandler {
public:
    typedef boost::shared_ptr<RequestHandler> Ptr;

    virtual void AsyncHandleRequest();
    virtual void HandleResponse(const boost::system::error_code& e);
    virtual ~RequestHandler();

    virtual bool Available() { return !session_; }
    virtual void Update(HttpProxySession::Ptr session) {
        session_ = session;
        request_ = session->Request();
        response_ = session->Response();
    }

protected:
    RequestHandler(HttpProxySession::Ptr session = HttpProxySession::Ptr());

    virtual HttpRequest *WrapRequest() = 0;
    virtual void ProcessResponse() = 0;

    virtual void reset() {
        session_.reset();
        request_ = NULL;
        response_ = NULL;
    }

    HttpProxySession::Ptr session_;
    HttpRequest *request_;
    HttpResponse *response_;
};

class DirectHandler : public RequestHandler {
    friend class RequestHandler;
    friend class RequestHandlerManager;
private:
    DirectHandler(HttpProxySession::Ptr session) : RequestHandler(session) {}

    virtual HttpRequest *WrapRequest() { return request_; }
    virtual void ProcessResponse() {}
};

class ProxyHandler : public RequestHandler {
    friend class RequestHandler;
    friend class RequestHandlerManager;
private:
    ProxyHandler(HttpProxySession::Ptr session) : RequestHandler(session) {}

    virtual HttpRequest *WrapRequest();
    virtual void ProcessResponse();
    void BuildProxyRequest();

    virtual void reset() {
        RequestHandler::reset();
        proxy_request_.reset();
    }

    std::auto_ptr<HttpRequest> proxy_request_;
};

#endif // REQUEST_HANDLER_H
