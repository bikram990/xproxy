#include <boost/bind.hpp>
#include "log.h"
#include "proxy_server.h"

ProxyServer::ProxyServer(short port)
    : signals_(service_), acceptor_(service_) {
    init(port);
    StartAccept();
}

void ProxyServer::Start() {
    service_.run();
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
    current_session_.reset(new HttpProxySession(service_, session_manager_));
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
        session_manager_.Start(current_session_);

    StartAccept();
}

void ProxyServer::OnStopSignalReceived() {
    acceptor_.close();
    session_manager_.StopAll();
}
