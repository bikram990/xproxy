#include "xproxy/log/log.hpp"
#include "xproxy/message/http/http_message.hpp"
#include "xproxy/plugin/proxy_plugin.hpp"

namespace xproxy {
namespace plugin {

void ProxyPlugin::onRequestHeaders(message::http::HttpMessage& request, net::SharedConnectionContext context) {
    // do nothing here
}

std::shared_ptr<message::http::HttpMessage> ProxyPlugin::onRequestMessage(message::http::HttpMessage& request, net::SharedConnectionContext context) {
    // TODO enhance this function, remove the hard coded strings
    std::string host;
    if (request.findHeader("Host", host)) {
        if (host == "twitter.com") {
            proxied_ = true;
            XDEBUG << "Host " << host << " is proxied.";
        }
    }

    if (!proxied_)
        return nullptr;

    memory::ByteBuffer buf;
    request.serialize(buf);

    request.reset();

    request.setField(message::http::HttpMessage::kRequestMethod, std::move(std::string("POST")));
    request.setField(message::http::HttpMessage::kRequestUri, std::move(std::string("/proxy")));
    request.setMajorVersion(1);
    request.setMinorVersion(1);
    request.addHeader("Host", "0x77ff.appspot.com")
           .addHeader("Connection", "keep-alive")
           .addHeader("User-Agent", "xProxy/0.01")
           .addHeader("Content-Length", std::to_string(buf.size()));
    if (context->https)
        request.addHeader("XProxy-Schema", "https://");

    request.appendBody(buf.data(), buf.size());

    context->https = true;
    // context->remote_host = "google.com.hk";
    context->remote_host = "google.org";
    context->remote_port = "443";
    context->server_ssl_setup = false;
    return nullptr;
}

void ProxyPlugin::onResponseHeaders(message::http::HttpMessage& response, net::SharedConnectionContext context) {
    if (!proxied_)
        return;

    memory::ByteBuffer buf;
    response.serialize(buf); // remove the proxy headers
}

void ProxyPlugin::onResponseMessage(message::http::HttpMessage& response, net::SharedConnectionContext context) {
    // do nothing here currently
}

int ProxyPlugin::priority() {
    return 1000;
}

} // namespace plugin
} // namespace xproxy
