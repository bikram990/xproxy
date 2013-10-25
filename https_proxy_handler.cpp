#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "https_proxy_handler.h"
#include "http_proxy_session.h"
#include "log.h"
#include "resource_manager.h"

HttpsProxyHandler::HttpsProxyHandler(HttpProxySession& session,
                                     HttpRequestPtr request)
    : session_(session),
      remote_ssl_context_(boost::asio::ssl::context::sslv23),
      origin_request_(request),
      client_(session.service(), request.get(), &remote_ssl_context_) {
    TRACE_THIS_PTR;
    init();
}

void HttpsProxyHandler::init() {
    local_ssl_context_.reset(new boost::asio::ssl::context(session_.service(), boost::asio::ssl::context::sslv23));
    local_ssl_context_->set_options(boost::asio::ssl::context::default_workarounds
                                    | boost::asio::ssl::context::no_sslv2
                                    | boost::asio::ssl::context::single_dh_use);
    ResourceManager::CertManager::CAPtr ca = ResourceManager::instance().GetCertManager().GetCertificate(origin_request_->host());
    ::SSL_CTX_use_certificate(local_ssl_context_->native_handle(), ca->cert);
    ::SSL_CTX_use_PrivateKey(local_ssl_context_->native_handle(), ca->key);
    ResourceManager::CertManager::DHParametersPtr dh = ResourceManager::instance().GetCertManager().GetDHParameters();
    ::SSL_CTX_set_tmp_dh(local_ssl_context_->native_handle(), dh->dh);
    local_ssl_socket_.reset(new ssl_socket_ref(session_.LocalSocket(), *local_ssl_context_));
}

void HttpsProxyHandler::HandleRequest() {
    XTRACE << "Received a HTTPS request, host: " << origin_request_->host()
           << ", port: " << origin_request_->port();

    static std::string response("HTTP/1.1 200 Connection Established\r\nProxy-Connection: Keep-Alive\r\n\r\n");
    boost::asio::async_write(local_ssl_socket_->next_layer(), boost::asio::buffer(response),
                             boost::bind(&HttpsProxyHandler::OnLocalSSLReplySent,
                                         boost::static_pointer_cast<HttpsProxyHandler>(shared_from_this()),
                                         boost::asio::placeholders::error));
}

void HttpsProxyHandler::OnLocalSSLReplySent(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to write SSL OK reply to local socket, message: " << e.message();
        session_.Terminate();
        return;
    }

    local_ssl_socket_->async_handshake(boost::asio::ssl::stream_base::server,
                                      boost::bind(&HttpsProxyHandler::OnLocalHandshaken,
                                                  boost::static_pointer_cast<HttpsProxyHandler>(shared_from_this()),
                                                  boost::asio::placeholders::error));
}

void HttpsProxyHandler::OnLocalHandshaken(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to handshake with local client, message: " << e.message();
        session_.Terminate();
        return;
    }

    origin_request_->reset();
    boost::asio::streambuf::mutable_buffers_type buf = local_buffer_.prepare(4096); // TODO hard code
    local_ssl_socket_->async_read_some(buf,
                                      boost::bind(&HttpsProxyHandler::OnLocalDataReceived,
                                                  boost::static_pointer_cast<HttpsProxyHandler>(shared_from_this()),
                                                  boost::asio::placeholders::error,
                                                  boost::asio::placeholders::bytes_transferred));
}

void HttpsProxyHandler::OnLocalDataReceived(const boost::system::error_code& e, std::size_t size) {
    if(e) {
        XWARN << "Failed to receive data from local socket, message: " << e.message();
        session_.Terminate();
        return;
    }

    local_buffer_.commit(size);

    XDEBUG << "Dump ssl encrypted data from local socket(size:" << size << "):\n"
           << "--------------------------------------------\n"
           << boost::asio::buffer_cast<const char *>(local_buffer_.data())
           << "\n--------------------------------------------";

    // TODO can we build on the original request?
    // TODO2 we should send the raw data directly, do not do parse and compose work
    HttpRequest::State result = *origin_request_ << local_buffer_;

    if(result == HttpRequest::kIncomplete) {
        XWARN << "This request is not complete, continue to read from the ssl socket.";
        boost::asio::streambuf::mutable_buffers_type buf = local_buffer_.prepare(2048); // TODO hard code
        local_ssl_socket_->async_read_some(buf,
                                          boost::bind(&HttpsProxyHandler::OnLocalDataReceived,
                                                      boost::static_pointer_cast<HttpsProxyHandler>(shared_from_this()),
                                                      boost::asio::placeholders::error,
                                                      boost::asio::placeholders::bytes_transferred));
        return;
    } else if(result == HttpRequest::kBadRequest) {
        XWARN << "Bad request: " << local_ssl_socket_->lowest_layer().remote_endpoint().address()
              << ":" << local_ssl_socket_->lowest_layer().remote_endpoint().port();
        // TODO here we should write a bad request response back
        session_.Terminate();
        return;
    }

    BuildProxyRequest(proxy_request_);
    client_.host(ResourceManager::instance().GetServerConfig().GetGAEServerDomain()).port(443).request(&proxy_request_)
           .AsyncSendRequest(boost::bind(&HttpsProxyHandler::OnResponseReceived,
                                         boost::static_pointer_cast<HttpsProxyHandler>(shared_from_this()), _1, _2));
    XTRACE << "Request is sent asynchronously.";
}

void HttpsProxyHandler::BuildProxyRequest(HttpRequest& request) {
    // TODO improve this, refine the headers
    boost::asio::streambuf& origin_body_buf = origin_request_->OutboundBuffer();

    request.method("POST").uri("/proxy").major_version(1).minor_version(1)
           .AddHeader("Host", ResourceManager::instance().GetServerConfig().GetGAEAppId() + ".appspot.com")
           .AddHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:24.0) Gecko/20100101 Firefox/24.0")
           .AddHeader("Connection", "close")
           .AddHeader("Content-Length", boost::lexical_cast<std::string>(origin_body_buf.size()))
           .body(origin_body_buf);
}

void HttpsProxyHandler::OnResponseReceived(const boost::system::error_code& e, HttpResponse *response) {
    if(e && e != boost::asio::error::eof) {
        XWARN << "Failed to send request, message: " << e.message();
        session_.Terminate();
        return;
    }

    XTRACE << "Response is back, status line: " << response->status_line();

    boost::asio::async_write(*local_ssl_socket_, response->body(),
                             boost::bind(&HttpsProxyHandler::OnLocalDataSent,
                                         boost::static_pointer_cast<HttpsProxyHandler>(shared_from_this()),
                                         boost::asio::placeholders::error));
}

void HttpsProxyHandler::OnLocalDataSent(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to write response to local socket, message: " << e.message();
        session_.Terminate();
        return;
    }

    XDEBUG << "Content written to local socket.";

    if(!e) {
        boost::system::error_code ec;
        //local_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        //remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    }

    if(e != boost::asio::error::operation_aborted) {
        //manager_.Stop(shared_from_this());
        session_.Terminate();
    }
}
