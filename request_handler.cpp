#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include "http_client.h"
#include "http_proxy_session.h"
#include "request_handler.h"
#include "resource_manager.h"

RequestHandler::RequestHandler(HttpProxySession::Ptr session,
                               HttpRequest::Ptr request,
                               HttpResponse::Ptr response,
                               handler_type handler)
    : session_(session), request_(request), response_(response),
      handler_(handler), inited_(false) {
    TRACE_THIS_PTR;
}

RequestHandler::~RequestHandler() {
    TRACE_THIS_PTR;
}

RequestHandler *RequestHandler::CreateHandler(HttpProxySession::Ptr session,
                                              HttpRequest::Ptr request,
                                              HttpResponse::Ptr response,
                                              handler_type handler) {
    // TODO may add socks handler here
    bool proxy = ResourceManager::instance().GetRuleConfig().RequestProxy(request->host());
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

    HttpRequest::Ptr request = WrapRequest();
    if(!request || !client_)
        handler_(boost::asio::error::invalid_argument);
    else
        client_->host(request->host()).port(request->port()).request(request)
                .AsyncSendRequest(boost::bind(&RequestHandler::HandleResponse,
                                              shared_from_this(), _1));
}

void RequestHandler::HandleResponse(const boost::system::error_code& e) {
    if(e && e != boost::asio::error::eof) {
        handler_(e);
        return;
    }

    ProcessResponse();
    handler_(e);
}

HttpRequest::Ptr DirectHandler::WrapRequest() {
    return request_;
}

void DirectHandler::init() {
    if(session_->mode() == HttpProxySession::HTTPS)
        remote_ssl_context_.reset(new boost::asio::ssl::context(boost::asio::ssl::context::sslv23));
    client_.reset(new HttpClient(session_->service(), request_, response_, remote_ssl_context_.get()));
    inited_ = true;
}

HttpRequest::Ptr ProxyHandler::WrapRequest() {
    proxy_request_.reset(new HttpRequest());
    BuildProxyRequest();
    return proxy_request_;
}

void ProxyHandler::ProcessResponse() {
    // TODO improve this function here
    if(response_->body().size() <= 0) {
        XERROR << "No response from remote proxy server.";
        return;
    }
    std::istream in(&response_->body());
    std::getline(in, response_->status_line());
    response_->status_line().erase(response_->status_line().size() - 1); // remove the last \r
    response_->ResetHeaders();
    std::string header;
    while(std::getline(in, header)) {
        if(header == "\r")
            break;

        std::string::size_type sep_idx = header.find(':');
        if(sep_idx == std::string::npos) {
            XWARN << "Invalid header: " << header;
            continue;
        }

        std::string name = header.substr(0, sep_idx);
        std::string value = header.substr(sep_idx + 1, header.length() - 1 - name.length() - 1); // remove the last \r
        boost::algorithm::trim(name);
        boost::algorithm::trim(value);
        if(name == "Set-Cookie") {
            std::string s("_e_0Pho_1=deleted; expires=Thu, 01-Jan-1970 00:00:01 GMT; path=/; domain=.facebook.com; httponly, act=deleted; expires=Thu, 01-Jan-1970 00:00:01 GMT; path=/; domain=.facebook.com; httponly, c_user=100004434630773; expires=Thu, 28-Nov-2013 11:19:09 GMT; path=/; domain=.facebook.com; secure, csm=2; expires=Thu, 28-Nov-2013 11:19:09 GMT; path=/; domain=.facebook.com, datr=7HVvUiWeQITVcvcKWnsRqqi0; expires=Thu, 29-Oct-2015 11:19:09 GMT; path=/; domain=.facebook.com; httponly, fr=0kZn8FVv5OxKw4Asm.AWXIzOEoQ71ftBfaITdeEzS9KMA.BSb5mt.dr.AAA.AWWtaqhk; expires=Thu, 28-Nov-2013 11:19:09 GMT; path=/; domain=.facebook.com; httponly, lu=ggjOhk2OdXT1Y5T7-8wq2aOw; expires=Thu, 29-Oct-2015 11:19:09 GMT; path=/; domain=.facebook.com; httponly, s=Aa5FXyc4stM-lIOz.BSb5mt; expires=Thu, 28-Nov-2013 11:19:09 GMT; path=/; domain=.facebook.com; secure; httponly, wd=deleted; expires=Thu, 01-Jan-1970 00:00:01 GMT; path=/; domain=.facebook.com; httponly, xs=140%3AaGFeQwOWyUciOw%3A2%3A1383045549%3A11945; expires=Thu, 28-Nov-2013 11:19:09 GMT; path=/; domain=.facebook.com; secure; httponly");
            boost::regex expr(", ([^ =]+(?:=|$))");
            std::string replace("\r\nSet-Cookie: \\1");
            value = boost::regex_replace(value, expr, replace);
            response_->AddHeader(name, value);
        } else {
            response_->AddHeader(name, value);
        }
    }
    response_->body_lenth(response_->body().size());
}

void ProxyHandler::init() {
    if(session_->mode() == HttpProxySession::HTTPS)
        remote_ssl_context_.reset(new boost::asio::ssl::context(boost::asio::ssl::context::sslv23));
    client_.reset(new HttpClient(session_->service(), request_, response_, remote_ssl_context_.get()));
    inited_ = true;
}

inline void ProxyHandler::BuildProxyRequest() {
    // TODO improve this, refine the headers
    boost::asio::streambuf& origin_body_buf = request_->OutboundBuffer();

    proxy_request_->host(ResourceManager::instance().GetServerConfig().GetGAEServerDomain())
           .port(443).method("POST").uri("/proxy").major_version(1).minor_version(1)
           .AddHeader("Host", ResourceManager::instance().GetServerConfig().GetGAEAppId() + ".appspot.com")
           .AddHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:24.0) Gecko/20100101 Firefox/24.0")
           .AddHeader("Connection", "close")
           .AddHeader("Content-Length", boost::lexical_cast<std::string>(origin_body_buf.size()))
           .body(origin_body_buf);
    if(session_->mode() == HttpProxySession::HTTPS)
        proxy_request_->AddHeader("XProxy-Schema", "https://");
}
