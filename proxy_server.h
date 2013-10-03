#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include "http_proxy_session_manager.h"


class HttpProxyServer : private boost::noncopyable {
public:
    HttpProxyServer(int port = 7077);
    ~HttpProxyServer();

    void Start();
    void Stop();

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

#endif // PROXY_SERVER_H
