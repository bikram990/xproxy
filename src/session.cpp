#include <boost/lexical_cast.hpp>
#include "client_connection.h"
#include "http_request.hpp"
#include "http_response.hpp"
#include "proxy_server.h"
#include "resource_manager.h"
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
    try {
        client_connection_->socket().close();
        server_connection_->socket().close();
    } catch(boost::system::system_error& e) {
        XERROR << "Error occurred during close socket, code: " << e.code().value()
               << ", message: " << e.code().message();
        // do nothing here
    }
}

void Session::OnRequestComplete(const HttpMessage& request) {
    auto method = request.GetField(HttpMessage::kRequestMethod);
    if (method.length() == 7 && method[0] == 'C' && method[1] == 'O') {
        const static std::string ssl_response("HTTP/1.1 200 Connection Established\r\n"
                                              "Content-Length: 0\r\n"
                                              "Connection: keep-alive\r\n"
                                              "Proxy-Connection: keep-alive\r\n\r\n");
        https_ = true;

        client_connection_->WriteSSLReply(ssl_response);
        return;
    }

    is_proxied_ = ResourceManager::GetRuleConfig().RequestProxy(client_connection_->host());
    if(!is_proxied_) {
        server_connection_->host(client_connection_->host());
        server_connection_->port(client_connection_->port());
        // server_connection_->host("10.64.1.186");
        // server_connection_->port(8080);

        // server_connection_->write(request);
        server_connection_->write(request, [](const HttpMessage& req, boost::asio::streambuf& buf) -> bool {
            req.serialize(buf, [](boost::asio::streambuf& buf, const char *data, std::size_t size) {
                auto copied = boost::asio::buffer_copy(buf.prepare(size), boost::asio::buffer(data, size));
                assert(copied == size);
                buf.commit(copied);
                return copied;
            });
            return true;
        });
        return;
    }

    boost::asio::streambuf proxy_request;
    boost::asio::streambuf body;
    request.serialize(body, [](boost::asio::streambuf& buf, const char *data, std::size_t size) {
        auto copied = boost::asio::buffer_copy(buf.prepare(size), boost::asio::buffer(data, size));
        assert(copied == size);
        buf.commit(copied);
        return copied;
    });

    std::ostream pr(&proxy_request);
    pr << "POST /proxy HTTP/1.1\r\n"
       << "Host: " << ResourceManager::GetServerConfig().GetGAEAppId() << ".appspot.com\r\n"
       << "Connection: keep-alive\r\n"
       << "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:24.0) Gecko/20100101 Firefox/24.0\r\n"
       << "Content-Length: " << body.size() << "\r\n";
    if (https_)
        pr << "XProxy-Schema: https://\r\n";
    pr << "\r\n";
    boost::asio::buffer_copy(proxy_request.prepare(body.size()), body.data());
    proxy_request.commit(body.size());

    XDEBUG_WITH_ID << "Proxy request:\n"
                   << std::string(boost::asio::buffer_cast<const char*>(proxy_request.data()), proxy_request.size());

    server_connection_->host(ResourceManager::GetServerConfig().GetGAEServerDomain());
    server_connection_->port(443);
    https_ = true;
    server_connection_->write(proxy_request, [](const boost::asio::streambuf& req, boost::asio::streambuf& buf) -> bool {
        boost::asio::buffer_copy(buf.prepare(req.size()), req.data());
        buf.commit(req.size());
        return true;
    });
}

void Session::OnResponse(const HttpMessage& response) {
    if (!is_proxied_) {
        client_connection_->write(response);
        return;
    }
    client_connection_->write(response, [](const HttpMessage& resp, boost::asio::streambuf& buf) -> bool {
        resp.serialize(buf, [](boost::asio::streambuf& buf, const char *data, std::size_t size) {
            auto copied = boost::asio::buffer_copy(buf.prepare(size), boost::asio::buffer(data, size));
            assert(copied == size);
            buf.commit(copied);
            return copied;
        });
        return true;
    });
}

void Session::OnResponseComplete(const HttpMessage& response) {
    OnResponse(response);

    server_connection_->OnMessageExchangeComplete();
    client_connection_->OnMessageExchangeComplete();

}
