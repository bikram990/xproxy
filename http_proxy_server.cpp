#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "log.h"
#include "http_proxy_server.h"

HttpProxyServer::HttpProxyServer(const std::string& address, int port)
    : service_(), signals_(service_), acceptor_(service_) {
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
    // signals_.add(SIGQUIT); // TODO is this needed?
    signals_.async_wait(boost::bind(&HttpProxyServer::HandleStop, this));

    using namespace boost::asio::ip;
    tcp::resolver r(service_);
    tcp::resolver::query q(address, boost::lexical_cast<std::string>(port));
    tcp::endpoint e = *r.resolve(q);
    acceptor_.open(e.protocol());
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
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
    // TODO implement me
}

void HttpProxyServer::HandleAccept(const boost::system::error_code &e) {
    // TODO implement me
}

void HttpProxyServer::HandleStop() {
    acceptor_.close();
    // TODO implement me
}
