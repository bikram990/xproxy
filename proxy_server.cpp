#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include "common.h"
#include "http_client_manager.h"
#include "http_proxy_session_manager.h"
#include "log.h"
#include "proxy_server.h"

namespace {
    static Singleton<ProxyServer> server_;
}

ProxyServer& ProxyServer::instance() {
    return server_.get();
}

ProxyServer::ProxyServer(short port,
                         int main_thread_count,
                         int fetch_thread_count)
    : state_(kUninitialized), port_(port),
      main_thread_pool_size_(main_thread_count), fetch_thread_pool_size_(fetch_thread_count),
      signals_(main_service_), acceptor_(main_service_) {}

void ProxyServer::start() {
    if(state_ == kUninitialized) {
        if(!init()) {
            XFATAL << "Failed to initialzie proxy server.";
            return;
        }
    }

    StartAccept();

    main_keeper_.reset(new boost::asio::io_service::work(main_service_));
    fetch_keeper_.reset(new boost::asio::io_service::work(fetch_service_));

    std::vector<boost::shared_ptr<boost::thread> > main_threads;
    std::vector<boost::shared_ptr<boost::thread> > fetch_threads;

    for(int i = 0; i < main_thread_pool_size_; ++i)
        main_threads.push_back(boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&boost::asio::io_service::run, &main_service_))));

    for(int i = 0; i < fetch_thread_pool_size_; ++i)
        fetch_threads.push_back(boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&boost::asio::io_service::run, &fetch_service_))));

    for(std::vector<boost::shared_ptr<boost::thread> >::iterator it = main_threads.begin();
        it != main_threads.end(); it++)
        (*it)->join();

    for(std::vector<boost::shared_ptr<boost::thread> >::iterator it = fetch_threads.begin();
        it != fetch_threads.end(); it++)
        (*it)->join();

    state_ = kRunning;
    XINFO << "Proxy is started.";
}

bool ProxyServer::init() {
    if(!InitResourceManager() || !InitSessionManager() ||
       !InitHandlerManager() || !InitClientManager())
        return false;

    signals_.add(SIGINT);
    signals_.add(SIGTERM);
    // signals_.add(SIGQUIT); // TODO is this needed?
    signals_.async_wait(boost::bind(&ProxyServer::OnStopSignalReceived, this));

    boost::asio::ip::tcp::endpoint e(boost::asio::ip::tcp::v4(), port_);
    acceptor_.open(e.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(e);
    acceptor_.listen();

    return true;
}

void ProxyServer::StartAccept() {
    current_session_.reset(HttpProxySession::Create(main_service_, fetch_service_));
    acceptor_.async_accept(current_session_->LocalSocket(),
                           boost::bind(&ProxyServer::OnConnectionAccepted, this,
                                       boost::asio::placeholders::error));
}

void ProxyServer::OnConnectionAccepted(const boost::system::error_code &e) {
    if(!acceptor_.is_open()) {
        XWARN << "Acceptor is not open, error message: " << e.message();
        return;
    }
    if(!e)
        session_manager_->Start(current_session_);

    StartAccept();
}

void ProxyServer::OnStopSignalReceived() {
    acceptor_.close();
    session_manager_->StopAll();
    main_keeper_.reset();
    fetch_keeper_.reset();
}
