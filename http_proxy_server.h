#ifndef HTTP_PROXY_SERVER_H
#define HTTP_PROXY_SERVER_H

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "server.h"


class HttpProxyServer : public Server {
public:
    HttpProxyServer(const std::string& address = "localhost",
                    int port = 7077);
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
};

#endif // HTTP_PROXY_SERVER_H
