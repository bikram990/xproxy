#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "http_proxy_session.h"

HttpProxySession::HttpProxySession(boost::asio::io_service& local_service,
                                   HttpProxySessionManager& manager)
    : remote_service_(), local_socket_(local_service),
      remote_socket_(remote_service_), manager_(manager) {
}

HttpProxySession::~HttpProxySession() {
    // do nothing here
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
    std::cout << buffer_.data() << std::endl;
    // TODO implement me
}
