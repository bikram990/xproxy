#include <boost/bind.hpp>
#include "http_proxy_session.h"
#include "http_proxy_session_manager.h"
#include "resource_manager.h"

HttpProxySession::HttpProxySession(boost::asio::io_service& service,
                                   HttpProxySessionManager& manager)
    : state_(kWaiting), mode_(HTTP), persistent_(false),
      service_(service), strand_(service), local_socket_(service), manager_(manager) {
    TRACE_THIS_PTR;
}

HttpProxySession::~HttpProxySession() {
    TRACE_THIS_PTR;
}

void HttpProxySession::Stop() {
    local_socket_.close();
    //remote_socket_.close();

    // TODO maybe we remote this session from session manager here?
}

void HttpProxySession::Terminate() {
    manager_.Stop(shared_from_this());
}

void HttpProxySession::Start() {
    boost::asio::streambuf::mutable_buffers_type buf = local_buffer_.prepare(4096); // TODO hard code
    local_socket_.async_read_some(buf,
                                  strand_.wrap(boost::bind(&this_type::OnRequestReceived,
                                                           shared_from_this(),
                                                           boost::asio::placeholders::error,
                                                           boost::asio::placeholders::bytes_transferred)));
    state_ = kWaiting;
}

void HttpProxySession::OnRequestReceived(const boost::system::error_code &e,
                                         std::size_t size) {
    if(e) {
        XWARN << "Error occurred during receiving data from socket, message: " << e.message();
        Terminate();
        return;
    }

    local_buffer_.commit(size);

    XDEBUG << "Dump data from socket(size:" << size << ", session: " << this << "):\n"
           << "--------------------------------------------\n"
           << std::string(boost::asio::buffer_cast<const char *>(local_buffer_.data()), local_buffer_.size())
           << "\n--------------------------------------------";

    if(state_ == kWaiting)
        request_.reset(new HttpRequest());
    else if(state_ == kSSLWaiting)
        request_->reset();

    HttpRequest::State result = *request_ << local_buffer_;

    if(result == HttpRequest::kIncomplete) {
        ContinueReceiving();
        return;
    } else if(result == HttpRequest::kBadRequest) {
        XWARN << "Bad request: " << local_socket_.remote_endpoint().address()
              << ":" << local_socket_.remote_endpoint().port();
        // TODO here we should write a bad request response back
        state_ = kReplying;
        return;
    }

    std::string connection;
    if(request_->FindHeader("Proxy-Connection", connection)) {
        if(connection == "keep-alive")
            persistent_ = true;
        // TODO remove the Proxy-Connection header here
    }

    if(request_->method() == "CONNECT") {
        // TODO use stocked response
        mode_ = HTTPS;
        static std::string response("HTTP/1.1 200 Connection Established\r\nProxy-Connection: Keep-Alive\r\n\r\n");
        boost::asio::async_write(local_socket_, boost::asio::buffer(response),
                                 strand_.wrap(boost::bind(&this_type::OnSSLReplySent,
                                                          shared_from_this(),
                                                          boost::asio::placeholders::error)));
        state_ = kSSLReplying;
    } else {
        if(mode_ == HTTPS)
            request_->port(443);
        response_.reset(new HttpResponse());
        // TODO should the following use strand_ to wrap?
        handler_.reset(RequestHandler::CreateHandler(shared_from_this(), request_, response_, boost::bind(&this_type::OnResponseReceived, shared_from_this(), _1)));
        if(handler_)
            handler_->AsyncHandleRequest();
        XTRACE << "Request is sent asynchronously.";
        state_ = kFetching;
    }
}

void HttpProxySession::OnSSLReplySent(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Error occurred during writing SSL OK reply to socket, message: " << e.message();
        Terminate();
        return;
    }

    InitSSLContext();

    local_ssl_socket_->async_handshake(boost::asio::ssl::stream_base::server,
                                       strand_.wrap(boost::bind(&this_type::OnHandshaken,
                                                                shared_from_this(),
                                                                boost::asio::placeholders::error)));
    state_ = kSSLHandshaking;
}

void HttpProxySession::OnHandshaken(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Error occurred during handshaking with browser, message: " << e.message();
        Terminate();
        return;
    }

    //    request_->reset();
    local_ssl_socket_->async_read_some(local_buffer_.prepare(4096), // TODO hard code
                                       strand_.wrap(boost::bind(&this_type::OnRequestReceived,
                                                                shared_from_this(),
                                                                boost::asio::placeholders::error,
                                                                boost::asio::placeholders::bytes_transferred)));
    state_ = kSSLWaiting;
}

void HttpProxySession::OnResponseReceived(const boost::system::error_code& e) {
    if(e && e != boost::asio::error::eof) {
        XWARN << "Failed to send request, message: " << e.message();
        Terminate();
        return;
    }

    SendResponse(*response_);
}

void HttpProxySession::OnResponseSent(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Error occurred during writing response to socket, message: " << e.message();
        Terminate();
        return;
    }

    XTRACE << "Content written to local socket.";

    if(persistent_) {
        XTRACE << "This is a persistent connection, continue to use this session.";
        reset();
        Start();
    } else {
        local_socket_.close();
        Terminate();
    }
}

inline void HttpProxySession::ContinueReceiving() {
    XTRACE << "This request is incomplete, continue to read from socket...";
    switch(mode_) {
    case HTTP:
        local_socket_.async_read_some(local_buffer_.prepare(2048), // TODO hard code
                                      strand_.wrap(boost::bind(&this_type::OnRequestReceived,
                                                               shared_from_this(),
                                                               boost::asio::placeholders::error,
                                                               boost::asio::placeholders::bytes_transferred)));
        break;
    case HTTPS:
        local_ssl_socket_->async_read_some(local_buffer_.prepare(2048), // TODO hard code
                                           strand_.wrap(boost::bind(&this_type::OnRequestReceived,
                                                                    shared_from_this(),
                                                                    boost::asio::placeholders::error,
                                                                    boost::asio::placeholders::bytes_transferred)));
        break;
    default:
        XERROR << "Invalid mode: " << static_cast<int>(mode_);
    }

    state_ = kContinueTransferring;
}

inline void HttpProxySession::InitSSLContext() {
    local_ssl_context_.reset(new boost::asio::ssl::context(service_, boost::asio::ssl::context::sslv23));
    local_ssl_context_->set_options(boost::asio::ssl::context::default_workarounds
                                    | boost::asio::ssl::context::no_sslv2
                                    | boost::asio::ssl::context::single_dh_use);
    ResourceManager::CertManager::CAPtr ca = ResourceManager::instance().GetCertManager().GetCertificate(request_->host());
    ::SSL_CTX_use_certificate(local_ssl_context_->native_handle(), ca->cert);
    ::SSL_CTX_use_PrivateKey(local_ssl_context_->native_handle(), ca->key);
    ResourceManager::CertManager::DHParametersPtr dh = ResourceManager::instance().GetCertManager().GetDHParameters();
    ::SSL_CTX_set_tmp_dh(local_ssl_context_->native_handle(), dh->dh);
    local_ssl_socket_.reset(new ssl_socket_ref(local_socket_, *local_ssl_context_));
}

inline void HttpProxySession::SendResponse(HttpResponse& response) {
    XTRACE << "Response is back, status line: " << response.status_line();

    switch(mode_) {
    case HTTP:
        boost::asio::async_write(local_socket_, response.OutboundBuffer(),
                                 strand_.wrap(boost::bind(&this_type::OnResponseSent,
                                                          shared_from_this(),
                                                          boost::asio::placeholders::error)));
        break;
    case HTTPS:
        boost::asio::async_write(*local_ssl_socket_, response.OutboundBuffer(),
                                 strand_.wrap(boost::bind(&this_type::OnResponseSent,
                                                          shared_from_this(),
                                                          boost::asio::placeholders::error)));
        break;
    default:
        XERROR << "Invalid mode: " << static_cast<int>(mode_);
    }

    state_ = kReplying;
}

inline void HttpProxySession::reset() {
    if(persistent_) {
        state_ = kWaiting;
        mode_ = HTTP;
        persistent_ = false;
        request_.reset();
        response_.reset();
        handler_.reset();
    }
}
