#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include "log.h"
#include "http_proxy_server.h"

HttpProxyServer::HttpProxyServer(int port)
    : service_(), signals_(service_), acceptor_(service_),
      current_session_(), session_manager_() {
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
    // signals_.add(SIGQUIT); // TODO is this needed?
    signals_.async_wait(boost::bind(&HttpProxyServer::HandleStop, this));

    boost::asio::ip::tcp::endpoint e(boost::asio::ip::tcp::v4(), port);
    acceptor_.open(e.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(e);
    acceptor_.listen();

    StartAccept();
}

HttpProxyServer::~HttpProxyServer() {
}

void HttpProxyServer::Start() {
    service_.run();
}

void HttpProxyServer::Stop() {
    Log::warn("this function is not implemented");
}

void HttpProxyServer::StartAccept() {
    Log::debug("A new session is initialized.");
    current_session_.reset(new HttpProxySession(service_, session_manager_));
    acceptor_.async_accept(current_session_->LocalSocket(),
                           boost::bind(&HttpProxyServer::HandleAccept, this,
                                       boost::asio::placeholders::error));
}

void HttpProxyServer::HandleAccept(const boost::system::error_code &e) {
    if(!acceptor_.is_open()) {
        Log::warn("socket is not opened.");
        return;
    }
    if(!e)
        session_manager_.Start(current_session_);

    StartAccept();
}

void HttpProxyServer::HandleStop() {
    acceptor_.close();
    session_manager_.StopAll();
}
