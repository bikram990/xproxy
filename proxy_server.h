#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include "session.h"
#include "session_manager.h"
#include "singleton.h"

class ProxyServer : private boost::noncopyable {
    friend class Singleton<ProxyServer>;
public:
    static void Start() {
        instance().start();
    }

    static class SessionManager& SessionManager() {
        return *instance().session_manager_;
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

    boost::shared_ptr<Session> current_session_;
    std::unique_ptr<class SessionManager> session_manager_;
};

#endif // PROXY_SERVER_H
