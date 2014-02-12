#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include "session.h"
#include "session_manager.h"
#include "singleton.h"

class ProxyServer : private boost::noncopyable {
    friend class Singleton<ProxyServer>;
public:
    enum State {
        kUninitialized, kInitialized, kRunning, kStopped
    };

    static void Start() {
        instance().start();
    }

    static class SessionManager& SessionManager() {
        return *instance().session_manager_;
    }

    static boost::asio::io_service& service() {
        return instance().service_;
    }

    State state() const { return state_; }

private:

    static ProxyServer& instance();

    ProxyServer(unsigned short port = 7077, int thread_count = 5);

    void start();

    bool init();

    void StartAccept();
    void OnConnectionAccepted(const boost::system::error_code& e);
    void OnStopSignalReceived();

    State state_;

    unsigned short port_;

    int thread_pool_size_;

    boost::asio::io_service service_;

    std::unique_ptr<boost::asio::io_service::work> service_keeper_;  // keep service running

    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;

    boost::shared_ptr<Session> current_session_;
    std::unique_ptr<class SessionManager> session_manager_;
};

#endif // PROXY_SERVER_H
