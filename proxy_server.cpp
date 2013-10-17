#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "log.h"
#include "proxy_server.h"

ProxyServer::ProxyServer(short port)
    : signals_(service_), acceptor_(service_),
      ssl_context_(service_, boost::asio::ssl::context::sslv23) {
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
    ssl_context_.set_options(boost::asio::ssl::context::default_workarounds
                             | boost::asio::ssl::context::no_sslv2
                             | boost::asio::ssl::context::single_dh_use);
    ssl_context_.set_password_callback(boost::bind(&ProxyServer::GetSSLPassword, this));
    ssl_context_.use_certificate_chain_file("xproxy.pem");
    ssl_context_.use_private_key_file("xproxy.pem", boost::asio::ssl::context::pem);
    ssl_context_.use_tmp_dh_file("dh512.pem");

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
    current_session_.reset(new HttpProxySession(service_, ssl_context_, session_manager_));
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
