#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include "http_client.h"
#include "http_client_manager.h"
#include "http_proxy_session.h"
#include "http_request.h"
#include "proxy_server.h"
#include "request_handler.h"
#include "resource_manager.h"

RequestHandler::RequestHandler(HttpProxySession::Ptr session)
    : session_(session),
      request_(session->Request()), response_(session->Response()) {
    TRACE_THIS_PTR;
}

RequestHandler::~RequestHandler() {
    TRACE_THIS_PTR;
}

void RequestHandler::AsyncHandleRequest() {
    HttpRequest *request = WrapRequest();

    if(!request) {
//        session_->OnResponseReceived(boost::asio::error::invalid_argument);
        ProxyServer::MainService().post(boost::bind(&HttpProxySession::OnResponseReceived, session_, boost::asio::error::invalid_argument));
        reset();
    }
    else
        ProxyServer::ClientManager().AsyncHandleRequest(session_->mode(),
                                                        request,
                                                        response_,
                                                        boost::bind(&RequestHandler::HandleResponse,
                                                                    this, _1));
}

void RequestHandler::HandleResponse(const boost::system::error_code& e) {
    if(e && e != boost::asio::error::eof && !SSL_SHORT_READ(e)) {
//        session_->OnResponseReceived(e); // TODO change the interface name
        ProxyServer::MainService().post(boost::bind(&HttpProxySession::OnResponseReceived, session_, e));
        reset();
        return;
    }

    ProcessResponse();
//    session_->OnResponseReceived(e); // TODO change the interface name
    ProxyServer::MainService().post(boost::bind(&HttpProxySession::OnResponseReceived, session_, e));
    reset();
}

HttpRequest *ProxyHandler::WrapRequest() {
    proxy_request_.reset(new HttpRequest());
    BuildProxyRequest();
    return proxy_request_.get();
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

inline void ProxyHandler::BuildProxyRequest() {
    // TODO improve this, refine the headers
    boost::asio::streambuf& origin_body_buf = request_->OutboundBuffer();

    proxy_request_->host(ResourceManager::GetServerConfig().GetGAEServerDomain())
        .port(443).method("POST").uri("/proxy").major_version(1).minor_version(1)
        .AddHeader("Host", ResourceManager::GetServerConfig().GetGAEAppId() + ".appspot.com")
        .AddHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:24.0) Gecko/20100101 Firefox/24.0")
        .AddHeader("Connection", "close")
        .AddHeader("Content-Length", boost::lexical_cast<std::string>(origin_body_buf.size()))
        .body(origin_body_buf);
    if(session_->mode() == HttpProxySession::HTTPS)
        proxy_request_->AddHeader("XProxy-Schema", "https://");
}
