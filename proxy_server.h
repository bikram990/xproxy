#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include "http_proxy_session_manager.h"


class ProxyServer : private boost::noncopyable {
public:
    ProxyServer(short port = 7077,
                int main_thread_count = 2,
                int fetch_thread_count = 10);

    void Start();
    void Stop();

private:
    void init(short port);

    void StartAccept();
    void OnConnectionAccepted(const boost::system::error_code& e);
    void OnStopSignalReceived();

    int main_thread_pool_size_;
    int fetch_thread_pool_size_;

    std::auto_ptr<boost::asio::io_service::work> fetch_keeper_; // keep fetch service running

    boost::asio::io_service main_service_;  // communicate with browser
    boost::asio::io_service fetch_service_; // communicate with remote server
    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;

    HttpProxySession::Ptr current_session_;
    HttpProxySessionManager session_manager_;
};

#endif // PROXY_SERVER_H
