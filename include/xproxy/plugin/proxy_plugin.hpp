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

    virtual void onRequestHeaders(message_type& request, net::SharedConnectionContext context);
    virtual message_ptr onRequestMessage(message_type& request, net::SharedConnectionContext context);
    virtual void onResponseHeaders(message_type& response, net::SharedConnectionContext context);
    virtual void onResponseMessage(message_type& response, net::SharedConnectionContext context);
    virtual int requestPriority();
    virtual int responsePriority();

    DEFAULT_VIRTUAL_DTOR(ProxyPlugin);

    ProxyPlugin() : proxied_(false) {}

private:
    bool proxied_;
};

} // namespace plugin
} // namespace xproxy

#endif // BUILTIN_PLUGINS_HPP
