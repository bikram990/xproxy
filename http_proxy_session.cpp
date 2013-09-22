#include <boost/bind.hpp>
#include "http_proxy_session.h"
#include "http_proxy_session_manager.h"
#include "log.h"

HttpProxySession::HttpProxySession(boost::asio::io_service& service,
                                   HttpProxySessionManager& manager)
    : service_(service), local_socket_(service),
      remote_socket_(service), manager_(manager) {
    TRACE_THIS_PTR;
}

HttpProxySession::~HttpProxySession() {
    TRACE_THIS_PTR;
}

void HttpProxySession::Stop() {
    local_socket_.close();
    remote_socket_.close();

    // TODO maybe we remote this session from session manager here?
}

void HttpProxySession::Terminate() {
    manager_.Stop(shared_from_this());
}

void HttpProxySession::Start() {
    local_socket_.async_read_some(
                boost::asio::buffer(local_buffer_),
                boost::bind(&HttpProxySession::HandleLocalRead,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
}

void HttpProxySession::HandleLocalRead(const boost::system::error_code &e,
                                       std::size_t size) {
    if(!handler_)
        handler_.reset(RequestHandler::CreateHandler(local_buffer_.data(), size, *this, service_, local_socket_, remote_socket_));
    if(handler_)
        handler_->HandleRequest(local_buffer_.data(), size);
}
