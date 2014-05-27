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

void PluginChain::addPlugin(MessagePluginPtr plugin) {
    if (plugin)
        plugins_.push_back(plugin);
}

} // namespace plugin
} // namespace xproxy
