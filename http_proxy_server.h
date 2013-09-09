#ifndef HTTP_PROXY_SERVER_H
#define HTTP_PROXY_SERVER_H

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "server.h"
#include "http_proxy_session_manager.h"


class HttpProxyServer : public Server {
public:
    HttpProxyServer(int port = 7077);
    virtual ~HttpProxyServer();

    virtual void Start();
    virtual void Stop();

private:
    void StartAccept();
    void HandleAccept(const boost::system::error_code& e);
    void HandleStop();

    boost::asio::io_service service_;
    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;

    HttpProxySessionPtr current_session_;
    HttpProxySessionManager session_manager_;
};

#endif // HTTP_PROXY_SERVER_H
