#include <boost/bind.hpp>
#include "http_proxy_session.h"
#include "http_proxy_session_manager.h"
#include "proxy_server.h"
#include "request_handler.h"
#include "request_handler_manager.h"
#include "resource_manager.h"

boost::atomic<std::size_t> HttpProxySession::counter_(0);

HttpProxySession::HttpProxySession(boost::asio::io_service& service)
    : id_(counter_), state_(kWaiting), mode_(HTTP), persistent_(false),
      strand_(service), local_socket_(Socket::Create(service)),
      timeout_(service), request_(new HttpRequest) {
    TRACE_THIS_PTR_WITH_ID;
}

HttpProxySession::~HttpProxySession() {
    TRACE_THIS_PTR_WITH_ID;
}

void HttpProxySession::Stop() {
    local_socket_->close();
    //remote_socket_.close();

    // TODO maybe we remote this session from session manager here?
}

void HttpProxySession::Terminate() {
    ProxyServer::SessionManager().Stop(shared_from_this());
}

void HttpProxySession::Start() {
    local_socket_->async_read_some(request_->InboundBuffer().prepare(4096), // TODO hard code
                                   strand_.wrap(boost::bind(&this_type::OnRequestReceived,
                                                            shared_from_this(),
                                                            boost::asio::placeholders::error,
                                                            boost::asio::placeholders::bytes_transferred)));
    state_ = kWaiting;
}

void HttpProxySession::OnRequestReceived(const boost::system::error_code &e,
                                         std::size_t size) {
    if(persistent_)
        timeout_.cancel(); // if this is a persistent connection, cancel the waiting timer

    if(e) {
        if(e == boost::asio::error::eof || SSL_SHORT_READ(e))
            XDEBUG_WITH_ID << "Browser closed the socket: " << local_socket_->to_string() << ".";
        else
            XERROR_WITH_ID << "Error occurred during receiving data from socket, message: " << e.message();
        Terminate();
        return;
    }

    boost::asio::streambuf& buffer = request_->InboundBuffer();
    buffer.commit(size);

    XDEBUG_WITH_ID << "Dump data from socket(size:" << size << ", session: " << this << "):\n"
                   << "--------------------------------------------\n"
                   << std::string(boost::asio::buffer_cast<const char *>(buffer.data()), buffer.size())
                   << "\n--------------------------------------------";

    if(state_ == kSSLWaiting)
        request_->reset();

    HttpRequest::State result = request_->consume();

    if(result == HttpRequest::kIncomplete) {
        ContinueReceiving();
        return;
    } else if(result == HttpRequest::kBadRequest) {
        XWARN_WITH_ID << "Bad request: " << local_socket_->to_string();
        // TODO here we should write a bad request response back
        state_ = kReplying;
        return;
    }

    std::string connection;
    if(request_->FindHeader("Proxy-Connection", connection)) {
        std::transform(connection.begin(), connection.end(), connection.begin(), std::ptr_fun<int, int>(std::tolower));
        if(connection == "keep-alive")
            persistent_ = true;
        // TODO remove the Proxy-Connection header here
    }

    if(request_->method() == "CONNECT") {
        // TODO use stocked response
        mode_ = HTTPS;
        static std::string response("HTTP/1.1 200 Connection Established\r\nProxy-Connection: Keep-Alive\r\n\r\n");
		local_socket_->async_write(boost::asio::buffer(response),
                                   strand_.wrap(boost::bind(&this_type::OnSSLReplySent,
                                                            shared_from_this(),
                                                            boost::asio::placeholders::error)));
        state_ = kSSLReplying;
    } else {
        if(mode_ == HTTPS)
            request_->port(443);
        response_.reset(new HttpResponse());
        ProxyServer::FetchService().post(boost::bind(&RequestHandlerManager::AsyncHandleRequest,
                                                     &ProxyServer::HandlerManager(),
                                                     shared_from_this()));
        //RequestHandlerManager::AsyncHandleRequest(shared_from_this());
        XTRACE_WITH_ID << "Request is sent asynchronously.";
        state_ = kFetching;
    }
}

void HttpProxySession::OnSSLReplySent(const boost::system::error_code& e) {
    if(e) {
        XWARN_WITH_ID << "Error occurred during writing SSL OK reply to socket, message: " << e.message();
        Terminate();
        return;
    }

    InitSSLContext();
    local_socket_->async_read_some(request_->InboundBuffer().prepare(4096), // TODO hard code
                                   strand_.wrap(boost::bind(&this_type::OnRequestReceived,
                                                            shared_from_this(), boost::asio::placeholders::error,
                                                            boost::asio::placeholders::bytes_transferred)));

    state_ = kSSLWaiting;
}

void HttpProxySession::OnResponseReceived(const boost::system::error_code& e) {
    if(e && e != boost::asio::error::eof && !SSL_SHORT_READ(e)) {
        XERROR_WITH_ID << "Error occurred during sending request to remote server, message: " << e.message();
        Terminate();
        return;
    }

    SendResponse(*response_);
}

void HttpProxySession::OnResponseSent(const boost::system::error_code& e) {
    if(e) {
        XWARN_WITH_ID << "Error occurred during writing response to socket, message: " << e.message();
        Terminate();
        return;
    }

    XTRACE_WITH_ID << "Content written to local socket.";

    if(persistent_) {
        XTRACE_WITH_ID << "This is a persistent connection, continue to use this session.";
        reset();
        Start();
    } else {
        local_socket_->close();
        Terminate();
    }
}

void HttpProxySession::OnTimeout(const boost::system::error_code& e) {
    if(e == boost::asio::error::operation_aborted)
        XDEBUG_WITH_ID << "The timeout timer is cancelled.";
    else if(e)
        XERROR_WITH_ID << "Error occurred with the timer, message: " << e.message();
    else if(timeout_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
        XDEBUG_WITH_ID << "Session timeout, close it.";
        Terminate();
    }
}

inline void HttpProxySession::ContinueReceiving() {
    XTRACE_WITH_ID << "This request is incomplete, continue to read from socket...";
    local_socket_->async_read_some(request_->InboundBuffer().prepare(2048), // TODO hard code
                                   strand_.wrap(boost::bind(&this_type::OnRequestReceived,
                                                            shared_from_this(),
                                                            boost::asio::placeholders::error,
                                                            boost::asio::placeholders::bytes_transferred)));
    state_ = kContinueTransferring;
}

inline void HttpProxySession::InitSSLContext() {
    ResourceManager::CertManager::CAPtr ca = ResourceManager::GetCertManager().GetCertificate(request_->host());
    ResourceManager::CertManager::DHParametersPtr dh = ResourceManager::GetCertManager().GetDHParameters();
    local_socket_->SwitchProtocol(kHttps, kServer, ca, dh);
}

inline void HttpProxySession::SendResponse(HttpResponse& response) {
    XTRACE_WITH_ID << "Response is back, status line: " << response.InitialLine();
    local_socket_->async_write(response.OutboundBuffer(),
                               strand_.wrap(boost::bind(&this_type::OnResponseSent,
                                                        shared_from_this(),
                                                        boost::asio::placeholders::error)));
    state_ = kReplying;
}

inline void HttpProxySession::reset() {
    if(persistent_) {
        state_ = kWaiting;
        persistent_ = false;
        request_.reset(new HttpRequest);
        response_.reset();
        timeout_.expires_from_now(boost::posix_time::seconds(kDefaultTimeout));
        timeout_.async_wait(boost::bind(&this_type::OnTimeout, this, boost::asio::placeholders::error));
    }
}
