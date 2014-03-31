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
      https_(false),
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
    auto req = std::static_pointer_cast<HttpRequest>(request);
    if (req->method().length() == 7
            && req->method()[0] == 'C' && req->method()[1] == 'O') {
        static std::string ssl_response("HTTP/1.1 200 Connection Established\r\n"
                                        "Content-Length: 0\r\n"
                                        "Connection: keep-alive\r\n"
                                        "Proxy-Connection: keep-alive\r\n\r\n");
        https_ = true;

        client_connection_->WriteSSLReply(ssl_response);
        return;
    }

    server_connection_->host(client_connection_->host());
    server_connection_->port(client_connection_->port());
    // server_connection_->host("10.64.1.186");
    // server_connection_->port(8080);

    server_connection_->write(request);
}

void Session::OnResponse(std::shared_ptr<HttpMessage> response) {
    client_connection_->write(response);
}

void Session::OnResponseComplete(std::shared_ptr<HttpMessage> response) {
    OnResponse(response);

    server_connection_->OnMessageExchangeComplete();
    client_connection_->OnMessageExchangeComplete();

}
