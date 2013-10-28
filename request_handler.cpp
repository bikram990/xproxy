#include <boost/lexical_cast.hpp>
#include "http_client.h"
#include "http_proxy_session.h"
#include "request_handler.h"
#include "resource_manager.h"

RequestHandler::RequestHandler(HttpProxySession& session,
                               HttpRequest& request,
                               HttpResponse& response,
                               handler_type handler)
    : session_(session), request_(request), response_(response),
      handler_(handler), remote_ssl_context_(NULL), client_(NULL), inited_(false) {
    TRACE_THIS_PTR;
}

RequestHandler::~RequestHandler() {
    TRACE_THIS_PTR;
    if(client_) {
        delete client_;
        client_ = NULL;
    }
    if(remote_ssl_context_) {
        delete remote_ssl_context_;
        remote_ssl_context_ = NULL;
    }
}

RequestHandler *RequestHandler::CreateHandler(HttpProxySession& session,
                                              HttpRequest& request,
                                              HttpResponse& response,
                                              handler_type handler) {
    // TODO may add socks handler here
    bool proxy = ResourceManager::instance().GetRuleConfig().RequestProxy(request.host());
    RequestHandler *h= NULL;
    if(proxy)
        h= new ProxyHandler(session, request, response, handler);
    else
        h= new DirectHandler(session, request, response, handler);
    return h;
}

void RequestHandler::AsyncHandleRequest() {
    if(!inited_)
        init();

    HttpRequest *request = WrapRequest();
    if(!request || !client_)
        handler_(boost::asio::error::invalid_argument);
    else
        client_->host(request->host()).port(request->port()).request(request).AsyncSendRequest(handler_);
}

HttpRequest *DirectHandler::WrapRequest() {
    return &request_;
}

void DirectHandler::init() {
    if(session_.mode() == HttpProxySession::HTTPS)
        remote_ssl_context_ = new boost::asio::ssl::context(boost::asio::ssl::context::sslv23);
    client_ = new HttpClient(session_.service(), &request_, response_, remote_ssl_context_);
    inited_ = true;
}

HttpRequest *ProxyHandler::WrapRequest() {
    proxy_request_ = new HttpRequest();
    BuildProxyRequest();
    return proxy_request_;
}

void ProxyHandler::init() {
    if(session_.mode() == HttpProxySession::HTTPS)
        remote_ssl_context_ = new boost::asio::ssl::context(boost::asio::ssl::context::sslv23);
    client_ = new HttpClient(session_.service(), &request_, response_, remote_ssl_context_);
    inited_ = true;
}

inline void ProxyHandler::BuildProxyRequest() {
    // TODO improve this, refine the headers
    boost::asio::streambuf& origin_body_buf = request_.OutboundBuffer();

    proxy_request_->host(ResourceManager::instance().GetServerConfig().GetGAEServerDomain())
           .port(443).method("POST").uri("/proxy").major_version(1).minor_version(1)
           .AddHeader("Host", ResourceManager::instance().GetServerConfig().GetGAEAppId() + ".appspot.com")
           .AddHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:24.0) Gecko/20100101 Firefox/24.0")
           .AddHeader("Connection", "close")
           .AddHeader("Content-Length", boost::lexical_cast<std::string>(origin_body_buf.size()))
           .body(origin_body_buf);
    if(session_.mode() == HttpProxySession::HTTPS)
        proxy_request_->AddHeader("XProxy-Schema", "https://");
}
