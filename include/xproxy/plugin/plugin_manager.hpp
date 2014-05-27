#ifndef PLUGIN_MANAGER_HPP
#define PLUGIN_MANAGER_HPP

#include "xproxy/util/singleton.hpp"
#include "xproxy/net/connection.hpp"

namespace xproxy {
namespace message {
namespace http { class HttpMessage; }
}

namespace plugin {

enum PluginMode {
    kRequest,
    kResponse,
    kBoth
};

class MessagePlugin {
public:
    DEFAULT_VIRTUAL_DTOR(MessagePlugin);

    virtual void onHeaders(message::http::HttpMessage& message,
                           net::SharedConnectionContext context) = 0;

    virtual void onMessage(message::http::HttpMessage& message,
                           net::SharedConnectionContext context) = 0;

    virtual int priority() = 0;
};

typedef std::shared_ptr<MessagePlugin> MessagePluginPtr;

class PluginManager {
    friend class util::Singleton<PluginManager>;
public:
    typedef MessagePluginPtr (*plugin_creator_type)();
    typedef std::list<MessagePluginPtr> plugin_container_type;
    typedef std::list<plugin_creator_type> creator_container_type;

    static bool init();

    static PluginManager& instance();

    static void registerPlugin(MessagePluginPtr global_plugin, PluginMode mode);

    static void registerPlugin(plugin_creator_type creator, PluginMode mode);

    const plugin_container_type& requestGlobalPlugins() const { return request_global_plugins_; }

    const plugin_container_type& responseGlobalPlugins() const { return response_global_plugins_; }

    const creator_container_type& pluginCreators(PluginMode mode) {
        switch (mode) {
        case kRequest:
            return request_plugin_creators_;
        case kResponse:
            return response_plugin_creators_;
        case kBoth:
            return shared_plugin_creators_;
        }
    }

    DEFAULT_DTOR(PluginManager);

private:
    PluginManager() = default;

private:
    plugin_container_type request_global_plugins_;
    plugin_container_type response_global_plugins_;
    creator_container_type request_plugin_creators_;
    creator_container_type response_plugin_creators_;
    creator_container_type shared_plugin_creators_;

private:
    MAKE_NONCOPYABLE(PluginManager);
};

class PluginChain {
public:
    static void create(PluginChain **request_chain, PluginChain **response_chain);

    void onHeaders(message::http::HttpMessage& message,
                   net::SharedConnectionContext context);

    void onMessage(message::http::HttpMessage& message,
                   net::SharedConnectionContext context);

    void addPlugin(MessagePluginPtr plugin);

    DEFAULT_DTOR(PluginChain);

private:
    PluginChain() = default;

private:
    PluginManager::plugin_container_type plugins_;

private:
    MAKE_NONCOPYABLE(PluginChain);
};

} // namespace plugin
} // namespace xproxy

#endif // PLUGIN_MANAGER_HPP
