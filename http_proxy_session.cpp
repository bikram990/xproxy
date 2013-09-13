#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "log.h"
#include "http_proxy_session.h"
#include "http_proxy_session_manager.h"

HttpProxySession::HttpProxySession(boost::asio::io_service& local_service,
                                   HttpProxySessionManager& manager)
    : remote_service_(), local_socket_(local_service),
      remote_socket_(remote_service_), manager_(manager),
      request_() {
    TRACE_THIS_PTR;
}

HttpProxySession::~HttpProxySession() {
    TRACE_THIS_PTR;
}

boost::asio::ip::tcp::socket& HttpProxySession::LocalSocket() {
    return local_socket_;
}

boost::asio::ip::tcp::socket& HttpProxySession::RemoteSocket() {
    return remote_socket_;
}

void HttpProxySession::Start() {
    local_socket_.async_read_some(
                boost::asio::buffer(buffer_),
                boost::bind(&HttpProxySession::HandleRead,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
}

void HttpProxySession::Stop() {
    local_socket_.close();
    remote_socket_.close();
}

void HttpProxySession::HandleRead(const boost::system::error_code &e,
                                  std::size_t size) {
    // TODO dump the raw request to log according a configuration item
    XTRACE << "Raw request: " << std::endl << buffer_.data();
//    local_socket_.async_send(boost::asio::buffer(std::string("HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello Proxy!")),
//                             boost::bind(&HttpProxySession::HandleWrite,
//                                         shared_from_this(),
//                                         boost::asio::placeholders::error));
    // TODO implement me
    ResultType result = request_.BuildFromRaw(buffer_.data(), size);
    if(result == HttpProxyRequest::kComplete) {
        boost::asio::streambuf response;
        manager_.dispatcher().DispatchRequest(request_.host(),
                                              request_.port(),
                                              &request_,
                                              response);
        boost::asio::async_write(local_socket_, response,
                                 boost::bind(&HttpProxySession::HandleWrite,
                                             shared_from_this(),
                                             boost::asio::placeholders::error));
    } else if(result == HttpProxyRequest::kNotComplete) {
        local_socket_.async_read_some(
                    boost::asio::buffer(buffer_),
                    boost::bind(&HttpProxySession::HandleRead,
                                shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
    } else { // build error
        XWARN << "Bad request: " << local_socket_.remote_endpoint().address()
              << ":" << local_socket_.remote_endpoint().port();
        // TODO implement this
    }
}

void HttpProxySession::HandleWrite(const boost::system::error_code& e) {
    if(!e) {
        boost::system::error_code ec;
        local_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    }

    if(e != boost::asio::error::operation_aborted) {
        manager_.Stop(shared_from_this());
    }
}
