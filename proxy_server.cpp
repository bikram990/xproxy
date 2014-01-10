#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include "common.h"
#include "log.h"
#include "proxy_server.h"

namespace {
    static Singleton<ProxyServer> server_;
}

ProxyServer& ProxyServer::instance() {
    return server_.get();
}

ProxyServer::ProxyServer(unsigned short port,
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

    if(state_ == kRunning)
        return;

    StartAccept();

    main_keeper_.reset(new boost::asio::io_service::work(main_service_));
    fetch_keeper_.reset(new boost::asio::io_service::work(fetch_service_));

    std::vector<boost::shared_ptr<boost::thread> > main_threads;
    std::vector<boost::shared_ptr<boost::thread> > fetch_threads;

    for(int i = 0; i < main_thread_pool_size_; ++i)
        main_threads.push_back(boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&boost::asio::io_service::run, &main_service_))));

    for(int i = 0; i < fetch_thread_pool_size_; ++i)
        fetch_threads.push_back(boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&boost::asio::io_service::run, &fetch_service_))));

    state_ = kRunning;
    XINFO << "Proxy is started.";

    for(std::vector<boost::shared_ptr<boost::thread> >::iterator it = fetch_threads.begin();
        it != fetch_threads.end(); it++)
        (*it)->join();

    for(std::vector<boost::shared_ptr<boost::thread> >::iterator it = main_threads.begin();
        it != main_threads.end(); it++)
        (*it)->join();
}

bool ProxyServer::init() {
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
    if(state_ == kStopped)
        return;

    current_session_.reset(Session::create(main_service_));
    acceptor_.async_accept(current_session_->ClientSocket(),
                           boost::bind(&ProxyServer::OnConnectionAccepted, this,
                                       boost::asio::placeholders::error));
}

void ProxyServer::OnConnectionAccepted(const boost::system::error_code &e) {
    if(!acceptor_.is_open()) {
        XWARN << "Acceptor is not open, error message: " << e.message();
        return;
    }

    XDEBUG << "A new session [id: " << current_session_->id() << "] is established, client address: ["
           << current_session_->ClientSocket().remote_endpoint().address() << ":"
           << current_session_->ClientSocket().remote_endpoint().port() << "].";

    if(!e)
        current_session_->start();

    StartAccept();
}

void ProxyServer::OnStopSignalReceived() {
    acceptor_.close();
    main_keeper_.reset();
    fetch_keeper_.reset();

    state_ = kStopped;
}
