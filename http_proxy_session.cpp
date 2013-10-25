#include <boost/bind.hpp>
#include "http_proxy_session.h"
#include "http_proxy_session_manager.h"
#include "log.h"

HttpProxySession::HttpProxySession(boost::asio::io_service& service,
                                   HttpProxySessionManager& manager)
    : service_(service), local_socket_(service), manager_(manager) {
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

    local_buffer_.commit(size);

    XDEBUG << "Dump data from local socket(size:" << size << "):\n"
           << "--------------------------------------------\n"
           << boost::asio::buffer_cast<const char *>(local_buffer_.data())
           << "\n--------------------------------------------";

    HttpRequestPtr request(boost::make_shared<HttpRequest>());
    HttpRequest::State result = *request << local_buffer_;

    if(result == HttpRequest::kIncomplete) {
        XWARN << "Not a complete request, continue reading...";
        local_socket_.async_read_some(local_buffer_.prepare(2048), // TODO hard code
                                      boost::bind(&HttpProxySession::OnRequestReceived,
                                                  shared_from_this(),
                                                  boost::asio::placeholders::error,
                                                  boost::asio::placeholders::bytes_transferred));
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
