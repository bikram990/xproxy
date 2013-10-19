#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "http_proxy_session.h"
#include "https_direct_handler.h"
#include "log.h"

HttpsDirectHandler::HttpsDirectHandler(HttpProxySession &session,
                                       HttpRequestPtr request)
    : session_(session), local_ssl_context_(session.LocalSSLContext()),
      local_ssl_socket_(session.LocalSocket(), local_ssl_context_),
      remote_ssl_context_(boost::asio::ssl::context::sslv23),
      request_(request),
      client_(session.service(), *request, &remote_ssl_context_) {
    TRACE_THIS_PTR;
}

HttpsDirectHandler::~HttpsDirectHandler() {
    TRACE_THIS_PTR;
}

void HttpsDirectHandler::HandleRequest() {
    XTRACE << "Received a HTTPS request, host: " << request_->host()
           << ", port: " << request_->port();

    static std::string response("HTTP/1.1 200 Connection Established\r\nProxy-Connection: Keep-Alive\r\n\r\n");
    boost::asio::async_write(local_ssl_socket_.next_layer(), boost::asio::buffer(response),
                             boost::bind(&HttpsDirectHandler::OnLocalSSLReplySent,
                                         shared_from_this(),
                                         boost::asio::placeholders::error));
}

void HttpsDirectHandler::OnLocalSSLReplySent(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to write SSL OK reply to local socket, message: " << e.message();
        session_.Terminate();
        return;
    }

    local_ssl_socket_.async_handshake(boost::asio::ssl::stream_base::server,
                                      boost::bind(&HttpsDirectHandler::OnLocalHandshaken,
                                                  shared_from_this(),
                                                  boost::asio::placeholders::error));
}

void HttpsDirectHandler::OnLocalHandshaken(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to handshake with local client, message: " << e.message();
        session_.Terminate();
        return;
    }

    boost::asio::streambuf::mutable_buffers_type buf = local_buffer_.prepare(4096); // TODO hard code
    local_ssl_socket_.async_read_some(buf,
                                      boost::bind(&HttpsDirectHandler::OnLocalDataReceived,
                                                  shared_from_this(),
                                                  boost::asio::placeholders::error,
                                                  boost::asio::placeholders::bytes_transferred));
}

void HttpsDirectHandler::OnLocalDataReceived(const boost::system::error_code& e, std::size_t size) {
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
    HttpRequest::State result = HttpRequest::BuildRequest(const_cast<char *>(boost::asio::buffer_cast<const char *>(local_buffer_.data())),
                                                          local_buffer_.size(), *request_);

    if(result != HttpRequest::kComplete) {
        XWARN << "This request is not complete, continue to read from the ssl socket.";
        boost::asio::streambuf::mutable_buffers_type buf = local_buffer_.prepare(2048); // TODO hard code
        local_ssl_socket_.async_read_some(buf,
                                          boost::bind(&HttpsDirectHandler::OnLocalDataReceived,
                                                      shared_from_this(),
                                                      boost::asio::placeholders::error,
                                                      boost::asio::placeholders::bytes_transferred));
        return;
    }

//    if(result == HttpRequest::kIncomplete) {
//        XWARN << "Not a complete request, but currently partial request is not supported.";
//        session_.Terminate();
//        return;
//    } else if(result == HttpRequest::kBadRequest) {
//        XWARN << "Bad request: " << local_ssl_socket_.lowest_layer().remote_endpoint().address()
//              << ":" << local_ssl_socket_.lowest_layer().remote_endpoint().port();
//        // TODO here we should write a bad request response back
//        session_.Terminate();
//        return;
//    }

    client_.AsyncSendRequest(boost::bind(&HttpsDirectHandler::OnResponseReceived,
                                         shared_from_this(), _1, _2));
    XTRACE << "Request is sent asynchronously.";
}

void HttpsDirectHandler::OnResponseReceived(const boost::system::error_code& e, HttpResponse *response) {
    if(e) {
        XWARN << "Failed to send request, message: " << e.message();
        session_.Terminate();
        return;
    }

    XTRACE << "Response is back, status line: " << response->status_line();

    boost::asio::async_write(local_ssl_socket_, response->OutboundBuffer(),
                             boost::bind(&HttpsDirectHandler::OnLocalDataSent,
                                         shared_from_this(),
                                         boost::asio::placeholders::error));
}

void HttpsDirectHandler::OnLocalDataSent(const boost::system::error_code& e) {
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
