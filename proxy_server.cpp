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

ProxyServer::ProxyServer(unsigned short port, int thread_count)
    : state_(kUninitialized), port_(port),
      thread_pool_size_(thread_count),
      signals_(service_), acceptor_(service_),
      session_manager_(new class SessionManager) {}

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

    service_keeper_.reset(new boost::asio::io_service::work(service_));

    std::vector<boost::shared_ptr<boost::thread>> threads;

    for(int i = 0; i < thread_pool_size_; ++i)
        threads.push_back(boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&boost::asio::io_service::run, &service_))));

    state_ = kRunning;
    XINFO << "Proxy is started.";

    for(std::vector<boost::shared_ptr<boost::thread> >::iterator it = threads.begin();
        it != threads.end(); it++)
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

    current_session_.reset(Session::create(service_, *session_manager_));
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
        session_manager_->start(current_session_);

    StartAccept();
}

void ProxyServer::OnStopSignalReceived() {
    acceptor_.close();
    service_keeper_.reset();

    state_ = kStopped;
}
