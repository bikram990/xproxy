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
    typedef boost::shared_ptr<RequestHandler> Ptr;
    typedef boost::function<void(const boost::system::error_code&)> handler_type;

    static RequestHandler *CreateHandler(boost::shared_ptr<HttpProxySession> session,
                                         HttpRequest::Ptr request,
                                         HttpResponse::Ptr response,
                                         handler_type handler);
    virtual void AsyncHandleRequest();
    virtual void HandleResponse(const boost::system::error_code& e);
    virtual ~RequestHandler();

protected:
    RequestHandler(boost::shared_ptr<HttpProxySession> session,
                   HttpRequest::Ptr request,
                   HttpResponse::Ptr response,
                   handler_type handler);

    virtual HttpRequest::Ptr WrapRequest() = 0;
    virtual void ProcessResponse() = 0;
    virtual void init() = 0;

    boost::shared_ptr<HttpProxySession> session_;
    HttpRequest::Ptr request_;
    HttpResponse::Ptr response_;

    handler_type handler_;
    boost::shared_ptr<boost::asio::ssl::context> remote_ssl_context_;
    boost::shared_ptr<HttpClient> client_;

    bool inited_;
};

class DirectHandler : public RequestHandler {
    friend class RequestHandler;
private:
    DirectHandler(boost::shared_ptr<HttpProxySession> session,
                  HttpRequest::Ptr request,
                  HttpResponse::Ptr response,
                  handler_type handler)
        : RequestHandler(session, request, response, handler) { init(); }

    virtual HttpRequest::Ptr WrapRequest();
    virtual void ProcessResponse() {}
    virtual void init();
};

class ProxyHandler : public RequestHandler {
    friend class RequestHandler;
private:
    ProxyHandler(boost::shared_ptr<HttpProxySession> session,
                 HttpRequest::Ptr request,
                 HttpResponse::Ptr response,
                 handler_type handler)
        : RequestHandler(session, request, response, handler) { init(); }

    virtual HttpRequest::Ptr WrapRequest();
    virtual void ProcessResponse();
    virtual void init();
    void BuildProxyRequest();

    HttpRequest::Ptr proxy_request_;
};

#endif // REQUEST_HANDLER_H
