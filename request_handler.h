#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <boost/asio/ssl.hpp>
#include <boost/enable_shared_from_this.hpp>

class HttpClient;
class HttpProxySession;
class HttpRequest;
class HttpResponse;

class RequestHandler : public boost::enable_shared_from_this<RequestHandler> {
public:
    static RequestHandler *CreateHandler(HttpProxySession& session);

    virtual void AsyncHandleRequest();
    virtual void HandleResponse(const boost::system::error_code& e);
    virtual ~RequestHandler();

protected:
    RequestHandler(HttpProxySession& session);

    virtual HttpRequest *WrapRequest() = 0;
    virtual void ProcessResponse() = 0;

    HttpProxySession& session_;
    HttpRequest *request_;
    HttpResponse *response_;
};

class DirectHandler : public RequestHandler {
    friend class RequestHandler;
private:
    DirectHandler(HttpProxySession& session) : RequestHandler(session) {}

    virtual HttpRequest *WrapRequest();
    virtual void ProcessResponse() {}
};

class ProxyHandler : public RequestHandler {
    friend class RequestHandler;
private:
    ProxyHandler(HttpProxySession& session) : RequestHandler(session) {}

    virtual HttpRequest *WrapRequest();
    virtual void ProcessResponse();
    void BuildProxyRequest();

    std::auto_ptr<HttpRequest> proxy_request_;
};

#endif // REQUEST_HANDLER_H
