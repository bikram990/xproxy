#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include "common.h"
#include "http_client_manager.h"
#include "http_proxy_session_manager.h"
#include "log.h"
#include "proxy_server.h"

ProxyServer::ProxyServer(short port,
                         int main_thread_count,
                         int fetch_thread_count)
    : main_thread_pool_size_(main_thread_count), fetch_thread_pool_size_(fetch_thread_count),
      fetch_keeper_(NULL),
      signals_(main_service_), acceptor_(main_service_) {
    init(port);
    StartAccept();
}

void ProxyServer::Start() {
    fetch_keeper_ = new boost::asio::io_service::work(fetch_service_);

    HttpClientManager::InitFetchService(&fetch_service_);

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
}

void ProxyServer::Stop() {
    XWARN << "Function not implemented.";
}

void ProxyServer::init(short port) {
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
    // signals_.add(SIGQUIT); // TODO is this needed?
    signals_.async_wait(boost::bind(&ProxyServer::OnStopSignalReceived, this));

    boost::asio::ip::tcp::endpoint e(boost::asio::ip::tcp::v4(), port);
    acceptor_.open(e.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(e);
    acceptor_.listen();
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
        HttpProxySessionManager::Start(current_session_);

    StartAccept();
}

void ProxyServer::OnStopSignalReceived() {
    acceptor_.close();
    HttpProxySessionManager::StopAll();
    CLEAN_MEMBER(fetch_keeper_); // delete the keeper to make fetch service finish
}
