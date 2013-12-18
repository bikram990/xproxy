#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include "client_connection_manager.h"
#include "filter_chain_manager.h"
#include "server_connection_manager.h"
#include "singleton.h"

class ProxyServer : private boost::noncopyable {
    friend class Singleton<ProxyServer>;
public:
    static void Start() {
        instance().start();
    }

    static class ServerConnectionManager& ServerConnectionManager() {
        return *instance().server_connection_manager_;
    }

    static class ClientConnectionManager& ClientConnectionManager() {
        return *instance().client_connection_manager_;
    }

    static class FilterChainManager& FilterChainManager() {
        return *instance().chain_manager_;
    }

    static boost::asio::io_service& MainService() {
        return instance().main_service_;
    }

    static boost::asio::io_service& FetchService() {
        return instance().fetch_service_;
    }

private:
    enum State {
        kUninitialized, kInitialized, kRunning, kStopped
    };

    static ProxyServer& instance();

    ProxyServer(unsigned short port = 7077,
                int main_thread_count = 2,
                int fetch_thread_count = 10);

    void start();

    bool init();

    bool InitServerConnectionManager() {
        server_connection_manager_.reset(new class ServerConnectionManager(fetch_service_));
        return true;
    }

    bool InitClientConnectionManager() {
        client_connection_manager_.reset(new class ClientConnectionManager());
        return true;
    }

    bool InitChainManager() {
        chain_manager_.reset(new class FilterChainManager());
        return true;
    }

    void StartAccept();
    void OnConnectionAccepted(const boost::system::error_code& e);
    void OnStopSignalReceived();

    State state_;

    unsigned short port_;

    int main_thread_pool_size_;
    int fetch_thread_pool_size_;

    boost::asio::io_service main_service_;  // communicate with browser
    boost::asio::io_service fetch_service_; // communicate with remote server

    std::unique_ptr<boost::asio::io_service::work> main_keeper_;  // keep main service running
    std::unique_ptr<boost::asio::io_service::work> fetch_keeper_; // keep fetch service running

    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;

    // HttpProxySession::Ptr current_session_;
    boost::shared_ptr<Connection> current_connection_;

    std::unique_ptr<class ServerConnectionManager> server_connection_manager_;
    std::unique_ptr<class ClientConnectionManager> client_connection_manager_;
    std::unique_ptr<class FilterChainManager> chain_manager_;
};

#endif // PROXY_SERVER_H
