#include <boost/thread.hpp>
#include "resource_manager.h"

const std::string ResourceManager::ServerConfig::kConfPortKey("basic.port");
const std::string ResourceManager::ServerConfig::kConfGAEAppIdKey("proxy_gae.app_id");
const std::string ResourceManager::ServerConfig::kConfGAEDomainKey("proxy_gae.domain");

namespace {
    static boost::mutex mutex_;
    static std::auto_ptr<ResourceManager> manager_;
}

ResourceManager& ResourceManager::instance() {
    if(!manager_.get()) {
        boost::lock_guard<boost::mutex> s(mutex_);
        if(!manager_.get())
            manager_.reset(new ResourceManager);
    }
    return *manager_;
}

ResourceManager::ResourceManager()
    : server_config_(new ServerConfig()), rule_config_(new RuleConfig()) {
    TRACE_THIS_PTR;
}

ResourceManager::~ResourceManager() {
    TRACE_THIS_PTR;
}
