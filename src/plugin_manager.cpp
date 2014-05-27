#include "xproxy/plugin/plugin_manager.hpp"

namespace xproxy {
namespace plugin {

namespace {
    util::Singleton<PluginManager> manager_;
}

bool PluginManager::init() {
    // TODO implement this function
    return true;
}

PluginManager& PluginManager::instance() {
    return manager_.get();
}

std::shared_ptr<PluginChain> PluginManager::create() {
    std::shared_ptr<PluginChain> chain(new PluginChain);

    std::for_each(global_plugins_.begin(), global_plugins_.end(), [chain] (const MessagePluginPtr& plugin) {
        chain->addPlugin(plugin);
    });

    std::for_each(plugin_creators_.begin(), plugin_creators_.end(), [chain] (const PluginManager::plugin_creator_type creator) {
        auto p = (*creator)();
        if (p)
            chain->addPlugin(p);
    });

    return chain;
}

void PluginManager::registerPlugin(MessagePluginPtr global_plugin) {
    if (global_plugin)
        instance().global_plugins_.push_back(global_plugin);
}

void PluginManager::registerPlugin(PluginManager::plugin_creator_type creator) {
    if (creator)
        instance().plugin_creators_.push_back(creator);
}

void PluginChain::onRequestHeaders(message::http::HttpMessage& request, net::SharedConnectionContext context) {
    if (plugins_.empty())
        return;

    for (auto it = plugins_.begin(); it != plugins_.end(); ++it) {
        (*it)->onRequestHeaders(request, context);
    }
}

std::shared_ptr<message::http::HttpMessage> PluginChain::onRequestMessage(message::http::HttpMessage& request, net::SharedConnectionContext context) {
    if (plugins_.empty())
        return nullptr;

    for (auto it = plugins_.begin(); it != plugins_.end(); ++it) {
        auto resp = (*it)->onRequestMessage(request, context);
        if (resp)
            return resp;
    }

    return nullptr;
}

void PluginChain::onResponseHeaders(message::http::HttpMessage& response, net::SharedConnectionContext context) {
    if (plugins_.empty())
        return;

    for (auto it = plugins_.begin(); it != plugins_.end(); ++it) {
        (*it)->onResponseHeaders(response, context);
    }
}

void PluginChain::onResponseMessage(message::http::HttpMessage& response, net::SharedConnectionContext context) {
    if (plugins_.empty())
        return;

    for (auto it = plugins_.begin(); it != plugins_.end(); ++it) {
        (*it)->onResponseMessage(response, context);
    }
}

void PluginChain::addPlugin(MessagePluginPtr plugin) {
    if (plugin)
        plugins_.push_back(plugin);
}

} // namespace plugin
} // namespace xproxy
