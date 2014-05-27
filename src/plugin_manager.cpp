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

void PluginManager::registerPlugin(MessagePluginPtr global_plugin, PluginMode mode) {
    if (!global_plugin)
        return;

    auto& mgr = manager_.get();
    switch (mode) {
    case kRequest:
        mgr.request_global_plugins_.push_back(global_plugin);
        break;
    case kResponse:
        mgr.response_global_plugins_.push_back(global_plugin);
    case kBoth:
        mgr.request_global_plugins_.push_back(global_plugin);
        mgr.response_global_plugins_.push_back(global_plugin);
        break;
    default:
        break;
    }
}

void PluginManager::registerPlugin(PluginManager::plugin_creator_type creator, PluginMode mode) {
    if (!creator)
        return;

    auto& mgr = manager_.get();
    switch (mode) {
    case kRequest:
        mgr.request_plugin_creators_.push_back(creator);
        break;
    case kResponse:
        mgr.response_plugin_creators_.push_back(creator);
        break;
    case kBoth:
        mgr.shared_plugin_creators_.push_back(creator);
        break;
    default:
        break;
    }
}

void PluginChain::create(PluginChain **request_chain, PluginChain **response_chain) {
    if (request_chain == nullptr || response_chain == nullptr)
        return;

    auto req = new PluginChain;
    auto res = new PluginChain;

    auto& req_global = manager_.get().requestGlobalPlugins();
    std::for_each(req_global.begin(), req_global.end(), [req] (const MessagePluginPtr& plugin) {
        req->addPlugin(plugin);
    });

    auto& res_global = manager_.get().responseGlobalPlugins();
    std::for_each(res_global.begin(), res_global.end(), [res] (const MessagePluginPtr& plugin) {
        res->addPlugin(plugin);
    });

    auto& req_creator = manager_.get().pluginCreators(kRequest);
    std::for_each(req_creator.begin(), req_creator.end(), [req] (const PluginManager::plugin_creator_type creator) {
        auto p = (*creator)();
        if (p)
            req->addPlugin(p);
    });

    auto& res_creator = manager_.get().pluginCreators(kResponse);
    std::for_each(res_creator.begin(), res_creator.end(), [res] (const PluginManager::plugin_creator_type creator) {
        auto p = (*creator)();
        if (p)
            res->addPlugin(p);
    });

    auto& shared_creator = manager_.get().pluginCreators(kBoth);
    std::for_each(shared_creator.begin(), shared_creator.end(), [req, res] (const PluginManager::plugin_creator_type creator) {
        auto p = (*creator)();
        if (p) {
            req->addPlugin(p);
            res->addPlugin(p);
        }
    });

    *request_chain = req;
    *response_chain = res;
}

void PluginChain::addPlugin(MessagePluginPtr plugin) {
    if (plugin)
        plugins_.push_back(plugin);
}

} // namespace plugin
} // namespace xproxy
