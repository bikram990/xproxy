#include <boost/lexical_cast.hpp>
#include "client_connection.h"
#include "http_request.h"
#include "http_response.h"
#include "proxy_server.h"
#include "server_connection.h"
#include "session.h"
#include "session_manager.h"

Session::Session(ProxyServer &server)
    : server_(server),
      manager_(server.SessionManager()),
      service_(server.service()),
      stopped_(false) {}

Session::~Session() {
    XDEBUG_WITH_ID << "Destructor called.";
}

void Session::init() {
    client_connection_.reset(new class ClientConnection(shared_from_this()));
    server_connection_.reset(new ServerConnection(shared_from_this()));
    client_connection_->init();
    server_connection_->init();
}

void Session::start() {
    XDEBUG_WITH_ID << "New session started, client connection id: "
                   << client_connection_->id() << ", server connection id: "
                   << server_connection_->id();
    client_connection_->read();
}

void Session::destroy() {
    XDEBUG_WITH_ID << "Session will be destroyed...";
    manager_.stop(shared_from_this());
}

void Session::stop() {
    stopped_ = true;
    client_connection_->socket().close();
    server_connection_->socket().close();
}

void Session::OnRequestComplete(std::shared_ptr<HttpMessage> request) {
    // TODO handle https CONNECT request here
    if (!request->serialize(server_connection_->OutBuffer())) {
        XERROR_WITH_ID << "Error occurred during serialization request.";
        destroy();
        return;
    }

    // TODO enhance the logic here
    std::string host;
    request->headers().find("Host", host);
   server_connection_->host(host);
   server_connection_->port(80);
    // server_connection_->host("10.64.1.186");
    // server_connection_->port(8080);

    server_connection_->write();
}

void Session::OnResponse(std::shared_ptr<HttpMessage> response) {
    if (!response->serialize(client_connection_->OutBuffer())) {
        XERROR_WITH_ID << "Error occurred during serialization response.";
        destroy();
        return;
    }

    client_connection_->write();
}

void Session::OnResponseComplete(std::shared_ptr<HttpMessage> response) {
    OnResponse(response);

    server_connection_->OnMessageExchangeComplete();
    client_connection_->OnMessageExchangeComplete();
}
