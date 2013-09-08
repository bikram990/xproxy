#ifndef HTTP_PROXY_SESSION_H
#define HTTP_PROXY_SESSION_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/shared_ptr.hpp>
#include "http_proxy_session_manager.h"


class HttpProxySession
        : public boost::enable_shared_from_this<HttpProxySession>,
          private boost::noncopyable {
public:
    HttpProxySession(boost::asio::io_service& local_service,
                     HttpProxySessionManager& manager);
    ~HttpProxySession();
    boost::asio::ip::tcp::socket& LocalSocket();
    boost::asio::ip::tcp::socket& RemoteSocket();
    void Start();
    void Stop();

private:
    boost::asio::io_service remote_service_;
    boost::asio::ip::tcp::socket local_socket_;
    boost::asio::ip::tcp::socket remote_socket_;
    HttpProxySessionManager& manager_;
};

typedef boost::shared_ptr<HttpProxySession> HttpProxySessionPtr;

#endif // HTTP_PROXY_SESSION_H
