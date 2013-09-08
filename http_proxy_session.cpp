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
