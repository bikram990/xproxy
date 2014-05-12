#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include "common.h"

class Connection;

class ProxyServer {
public:
    ProxyServer(unsigned short port = 7077);
    virtual ~ProxyServer() = default;

    void start();

private:
    bool init();

    void StartAccept();
    void OnConnectionAccepted(const boost::system::error_code& e);
    void OnStopSignalReceived();

    void StopAllConnections();

    unsigned short port_;
    boost::asio::io_service service_;
    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<Connection> current_client_connection_;
    std::set<std::shared_ptr<Connection>> client_connections_;

private:
    DISABLE_COPY_AND_ASSIGNMENT(ProxyServer);
};

#endif // PROXY_SERVER_H
