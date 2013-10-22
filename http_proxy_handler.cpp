#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "http_proxy_handler.h"
#include "http_proxy_session.h"
#include "log.h"
#include "resource_manager.h"

HttpProxyHandler::HttpProxyHandler(HttpProxySession& session,
                                   HttpRequestPtr request)
    : session_(session), local_socket_(session.LocalSocket()),
      remote_ssl_context_(boost::asio::ssl::context::sslv23),
      origin_request_(request),
      client_(session.service(), request.get(), &remote_ssl_context_) {
        TRACE_THIS_PTR;
}

void HttpProxyHandler::HandleRequest() {
    XTRACE << "New request received, host: " << origin_request_->host()
           << ", port: " << origin_request_->port();
    BuildProxyRequest(proxy_request_);
    client_.host(ResourceManager::instance().GetServerConfig().GetGAEServerDomain()).port(443).request(&proxy_request_)
           .AsyncSendRequest(boost::bind(&HttpProxyHandler::OnResponseReceived,
                                         boost::static_pointer_cast<HttpProxyHandler>(shared_from_this()), _1, _2));
    XTRACE << "Request is sent asynchronously.";
}

void HttpProxyHandler::BuildProxyRequest(HttpRequest& request) {
    // TODO improve this, refine the headers
    boost::asio::streambuf& origin_body_buf = origin_request_->OutboundBuffer();
    boost::asio::streambuf::const_buffers_type origin_body = origin_body_buf.data();
    std::size_t length = origin_body_buf.size();

    request.method("POST").uri("/proxy").major_version(1).minor_version(1)
           .AddHeader("Host", ResourceManager::instance().GetServerConfig().GetGAEAppId() + ".appspot.com")
           .AddHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:24.0) Gecko/20100101 Firefox/24.0")
           .AddHeader("Connection", "close")
           .AddHeader("Content-Length", boost::lexical_cast<std::string>(length))
           .body(boost::asio::buffers_begin(origin_body),
                 boost::asio::buffers_end(origin_body))
           .body_length(length);
}

void HttpProxyHandler::OnResponseReceived(const boost::system::error_code& e, HttpResponse *response) {
    if(e && e != boost::asio::error::eof) {
        XWARN << "Failed to send request, message: " << e.message();
        session_.Terminate();
        return;
    }

    XTRACE << "Response is back, status line: " << response->status_line();

    boost::asio::async_write(local_socket_, response->body(), // for proxied response, only write body
                             boost::bind(&HttpProxyHandler::OnLocalDataSent,
                                         boost::static_pointer_cast<HttpProxyHandler>(shared_from_this()),
                                         boost::asio::placeholders::error));
}

void HttpProxyHandler::OnLocalDataSent(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to write response to local socket, message: " << e.message();
        session_.Terminate();
        return;
    }

    XDEBUG << "Content written to local socket.";

    // TODO handle persistent connection here

    if(!e) {
        boost::system::error_code ec;
        local_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        //remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    }

    if(e != boost::asio::error::operation_aborted) {
        //manager_.Stop(shared_from_this());
        session_.Terminate();
    }
}
