#include <boost/bind.hpp>
#include "http_proxy_session.h"
#include "http_proxy_session_manager.h"
#include "log.h"

HttpProxySession::HttpProxySession(boost::asio::io_service& service,
                                   boost::asio::ssl::context& context,
                                   HttpProxySessionManager& manager)
    : service_(service), local_socket_(service),
      local_ssl_context_(context),
      /*remote_socket_(service),*/ manager_(manager) {
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
    local_socket_.async_read_some(
                boost::asio::buffer(local_buffer_),
                boost::bind(&HttpProxySession::OnRequestReceived,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
}

void HttpProxySession::OnRequestReceived(const boost::system::error_code &e,
                                       std::size_t size) {
    if(handler_) {
        XERROR << "A handler exists, it should never happen.";
        return;
    }

    XTRACE << "Dump data from local socket(size:" << size << "):\n"
           << "--------------------------------------------\n"
           << local_buffer_.data()
           << "\n--------------------------------------------";

    HttpRequestPtr request(boost::make_shared<HttpRequest>());
    HttpRequest::State result = HttpRequest::BuildRequest(local_buffer_.data(), size, *request);

    if(result == HttpRequest::kIncomplete) {
        XWARN << "Not a complete request, but currently partial request is not supported.";
        return;
    } else if(result == HttpRequest::kBadRequest) {
        XWARN << "Bad request: " << local_socket_.remote_endpoint().address()
              << ":" << local_socket_.remote_endpoint().port();
        // TODO here we should write a bad request response back
        return;
    }

    RequestHandler *h = RequestHandler::CreateHandler(*this, request);
    if(h) {
        handler_.reset(h);
        handler_->HandleRequest();
    } else {
        XERROR << "Request Handler cannot be created successfully.";
        Terminate();
    }
}
