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

void PluginManager::registerPlugin(std::shared_ptr<MessagePlugin> shared_plugin) {
    if (shared_plugin)
        shared_plugins_.push_back(shared_plugin);
}

void PluginManager::registerPlugin(std::shared_ptr<MessagePlugin> (*creator)()) {
    if (creator)
        plugin_creators_.push_back(creator);
}

} // namespace plugin
} // namespace xproxy
