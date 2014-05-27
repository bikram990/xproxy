#ifndef PLUGIN_MANAGER_HPP
#define PLUGIN_MANAGER_HPP

#include "xproxy/util/singleton.hpp"
#include "xproxy/net/connection.hpp"

namespace xproxy {
namespace message {
namespace http { class HttpMessage; }
}

namespace plugin {

enum PluginPriority {
    kHighest = 1000,
    kMidHi = 750,
    kMiddle = 500,
    kLoMid = 250,
    kLowest = 1
};

class MessagePlugin {
public:
    typedef message::http::HttpMessage message_type;
    typedef std::shared_ptr<message::http::HttpMessage> message_ptr;

    DEFAULT_VIRTUAL_DTOR(MessagePlugin);

    virtual void onRequestHeaders(message_type& request,
                                  net::SharedConnectionContext context) = 0;

    virtual message_ptr onRequestMessage(message_type& request,
                                         net::SharedConnectionContext context) = 0;

    virtual void onResponseHeaders(message_type& response,
                                   net::SharedConnectionContext context) = 0;

    virtual void onResponseMessage(message_type& response,
                                   net::SharedConnectionContext context) = 0;

    virtual int requestPriority() = 0;

    virtual int responsePriority() = 0;
};

typedef std::shared_ptr<MessagePlugin> MessagePluginPtr;

class PluginChain {
public:
    void onRequestHeaders(message::http::HttpMessage& request, net::SharedConnectionContext context);

    std::shared_ptr<message::http::HttpMessage> onRequestMessage(message::http::HttpMessage& request,
                                                                 net::SharedConnectionContext context);

    void onResponseHeaders(message::http::HttpMessage& response, net::SharedConnectionContext context);

    void onResponseMessage(message::http::HttpMessage& response, net::SharedConnectionContext context);

    void addPlugin(MessagePluginPtr plugin);

    DEFAULT_CTOR(PluginChain);
    DEFAULT_DTOR(PluginChain);

private:
    struct RequestComp {
        bool operator ()(const MessagePluginPtr& plugin1, const MessagePluginPtr& plugin2) {
            return plugin1->requestPriority() < plugin2->requestPriority();
        }
    };

    struct ResponseComp {
        bool operator ()(const MessagePluginPtr& plugin1, const MessagePluginPtr& plugin2) {
            return plugin1->responsePriority() < plugin2->responsePriority();
        }
    };

private:
    std::set<MessagePluginPtr, RequestComp> request_plugins_;
    std::set<MessagePluginPtr, ResponseComp> response_plugins_;

private:
    MAKE_NONCOPYABLE(PluginChain);
};

class PluginManager {
    friend class util::Singleton<PluginManager>;
public:
    typedef MessagePluginPtr (*plugin_creator_type)();
    typedef std::list<MessagePluginPtr> plugin_container_type;
    typedef std::list<plugin_creator_type> creator_container_type;

    static bool init();

    static PluginManager& instance();

    static void registerPlugin(MessagePluginPtr global_plugin);

    static void registerPlugin(plugin_creator_type creator);

    static std::shared_ptr<PluginChain> createChain() {
        return instance().create();
    }

    DEFAULT_DTOR(PluginManager);

private:
    DEFAULT_CTOR(PluginManager);

    std::shared_ptr<PluginChain> create();

private:
    plugin_container_type global_plugins_;
    creator_container_type plugin_creators_;

private:
    MAKE_NONCOPYABLE(PluginManager);
};

} // namespace plugin
} // namespace xproxy

#endif // PLUGIN_MANAGER_HPP
