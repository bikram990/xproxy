#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/noncopyable.hpp>
#include "http_proxy_session_manager.h"


class ProxyServer : private boost::noncopyable {
public:
    ProxyServer(short port = 7077);

    void Start();
    void Stop();

private:
    void init(short port);

    void StartAccept();
    void OnConnectionAccepted(const boost::system::error_code& e);
    void OnStopSignalReceived();

    // TODO enhance this function
    std::string GetSSLPassword() { return "xproxy"; }

    boost::asio::io_service service_;
    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;

    boost::asio::ssl::context ssl_context_;

    HttpProxySessionPtr current_session_;
    HttpProxySessionManager session_manager_;
};

#endif // PROXY_SERVER_H
