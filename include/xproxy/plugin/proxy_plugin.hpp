#ifndef BUILTIN_PLUGINS_HPP
#define BUILTIN_PLUGINS_HPP

#include "xproxy/plugin/plugin_manager.hpp"

namespace xproxy {
namespace plugin {

class ProxyPlugin : public MessagePlugin {
public:
    static MessagePluginPtr create() {
        return MessagePluginPtr(new ProxyPlugin);
    }

    virtual void onRequestHeaders(message::http::HttpMessage &request, net::SharedConnectionContext context);
    virtual std::shared_ptr<message::http::HttpMessage> onRequestMessage(message::http::HttpMessage &request, net::SharedConnectionContext context);
    virtual void onResponseHeaders(message::http::HttpMessage &response, net::SharedConnectionContext context);
    virtual void onResponseMessage(message::http::HttpMessage &response, net::SharedConnectionContext context);
    virtual int priority();

    DEFAULT_VIRTUAL_DTOR(ProxyPlugin);

    ProxyPlugin() : proxied_(false) {}

private:
    bool proxied_;
};

} // namespace plugin
} // namespace xproxy

#endif // BUILTIN_PLUGINS_HPP
