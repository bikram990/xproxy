#ifndef PROXY_SERVER_HPP
#define PROXY_SERVER_HPP

#include "common.hpp"
#include "net/connection.hpp"

namespace xproxy {

class ProxyServer {
public:
    ProxyServer(unsigned short port = 7077);
    DEFAULT_DTOR(ProxyServer);

    void start();

private:
    void init();
    void startAccept();

private:
    std::string port_;

    boost::asio::io_service service_;
    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;

    net::ConnectionPtr current_client_connection_;
    net::ConnectionManager client_connection_manager_;

private:
    MAKE_NONCOPYABLE(ProxyServer);
};

} // namespace xproxy

#endif // PROXY_SERVER_HPP
