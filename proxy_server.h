#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include "connection_manager.h"
#include "http_client_manager.h"
#include "request_handler_manager.h"
#include "singleton.h"

class ProxyServer : private boost::noncopyable {
    friend class Singleton<ProxyServer>;
public:
    static void Start() {
        instance().start();
    }

    static ConnectionManager& ConnectionManager() {
        return *instance().connection_manager_;
    }

    static RequestHandlerManager& HandlerManager() {
        return *instance().handler_manager_;
    }

    static HttpClientManager& ClientManager() {
        return *instance().client_manager_;
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

    ProxyServer(short port = 7077,
                int main_thread_count = 2,
                int fetch_thread_count = 10);

    void start();

    bool init();

    bool InitSessionManager() {
        connection_manager_.reset(new HttpProxySessionManager());
        return true;
    }

    bool InitHandlerManager() {
        handler_manager_.reset(new RequestHandlerManager());
        return true;
    }

    bool InitClientManager() {
        client_manager_.reset(new HttpClientManager(fetch_service_));
        return true;
    }

    void StartAccept();
    void OnConnectionAccepted(const boost::system::error_code& e);
    void OnStopSignalReceived();

    State state_;

    short port_;

    int main_thread_pool_size_;
    int fetch_thread_pool_size_;

    boost::asio::io_service main_service_;  // communicate with browser
    boost::asio::io_service fetch_service_; // communicate with remote server

    std::unique_ptr<boost::asio::io_service::work> main_keeper_;  // keep main service running
    std::unique_ptr<boost::asio::io_service::work> fetch_keeper_; // keep fetch service running

    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;

    // HttpProxySession::Ptr current_session_;
    ConnectionPtr current_connection_;

    std::unique_ptr<ConnectionManager> connection_manager_;
    std::unique_ptr<RequestHandlerManager> handler_manager_;
    std::unique_ptr<HttpClientManager> client_manager_;
};

#endif // PROXY_SERVER_H
