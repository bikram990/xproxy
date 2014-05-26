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
    kShared,
    kindividual
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

class PluginManager {
    friend class util::Singleton<PluginManager>;
public:
    typedef std::list<std::shared_ptr<MessagePlugin>> plugin_container_type;

    static bool init();

    static PluginManager& instance();

    void registerPlugin(std::shared_ptr<MessagePlugin> shared_plugin);

    void registerPlugin(std::shared_ptr<MessagePlugin> (*creator)());

    DEFAULT_DTOR(PluginManager);

private:
    PluginManager() = default;

private:
    plugin_container_type shared_plugins_;
    std::list<std::shared_ptr<MessagePlugin>(*)()> plugin_creators_;

private:
    MAKE_NONCOPYABLE(PluginManager);
};

class PluginChain {
public:
    void onHeaders(message::http::HttpMessage& message,
                   net::SharedConnectionContext context);

    void onMessage(message::http::HttpMessage& message,
                   net::SharedConnectionContext context);

    PluginChain();
    DEFAULT_DTOR(PluginChain);

private:
    PluginManager::plugin_container_type plugins_;

private:
    MAKE_NONCOPYABLE(PluginChain);
};

} // namespace plugin
} // namespace xproxy

#endif // PLUGIN_MANAGER_HPP
