#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <boost/noncopyable.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "log.h"

class ResourceManager : private boost::noncopyable {
    friend class std::auto_ptr<ResourceManager>;
public:
    class ServerConfig;
    class RuleConfig;

    static ResourceManager& instance();

    ServerConfig& GetServerConfig();
    RuleConfig& GetRuleConfig();

    bool init();

private:
    ResourceManager();
    ~ResourceManager();

    std::auto_ptr<ServerConfig> server_config_;
    std::auto_ptr<RuleConfig> rule_config_;
};

class ResourceManager::ServerConfig : private boost::noncopyable {
    friend class ResourceManager;
    friend class std::auto_ptr<ServerConfig>;
public:
    const static std::string kConfPortKey;
    const static std::string kConfGAEAppIdKey;
    const static std::string kConfGAEDomainKey;

    bool LoadConfig();
    bool LoadConfig(const std::string& conf_file);

    short GetProxyPort(short default_value = 7077);
    const std::string GetGAEAppId();
    const std::string GetGAEServerDomain(const std::string& default_value = "google.com.hk");

    template<typename TypeT>
    bool GetConfig(const std::string& path, TypeT& return_value) {
        try {
            return_value = conf_tree_.get<TypeT>(path);
        } catch(boost::property_tree::ptree_error& e) {
            XWARN << "Failed to read config, path: " << path
                  << ", reason: " << e.what();
            return false;
        }
        return true;
    }

private:
    ServerConfig(const std::string& conf_file = "xproxy.conf")
        : conf_file_(conf_file) {}
    ~ServerConfig() {}

    std::string conf_file_;
    boost::property_tree::ptree conf_tree_;
};

class ResourceManager::RuleConfig : private boost::noncopyable {
    friend class ResourceManager;
    friend class std::auto_ptr<RuleConfig>;
public:
    RuleConfig& operator<<(const std::string& domain);

    bool RequestProxy(const std::string& host);

private:
    RuleConfig() {}
    ~RuleConfig() {}

    std::vector<std::string> domains_;
};

inline ResourceManager::ServerConfig& ResourceManager::GetServerConfig() {
    return *server_config_;
}

inline ResourceManager::RuleConfig& ResourceManager::GetRuleConfig() {
    return *rule_config_;
}

inline bool ResourceManager::init() {
    return server_config_->LoadConfig();
}

inline bool ResourceManager::ServerConfig::LoadConfig() {
    try {
        boost::property_tree::ini_parser::read_ini(conf_file_, conf_tree_);
    } catch(boost::property_tree::ini_parser::ini_parser_error& e) {
        XFATAL << "Failed to load configuration from config file: " << conf_file_
               << ", reason: " << e.what();
        return false;
    }
    return true;
}

inline bool ResourceManager::ServerConfig::LoadConfig(const std::string& conf_file) {
    if(conf_file_ != conf_file)
        conf_file_ = conf_file;
    return LoadConfig();
}

inline short ResourceManager::ServerConfig::GetProxyPort(short default_value) {
    short result;
    return GetConfig(kConfPortKey, result) ? result : default_value;
}

inline const std::string ResourceManager::ServerConfig::GetGAEAppId() {
    std::string result;
    return GetConfig(kConfGAEAppIdKey, result) ? result : "";
}

inline const std::string ResourceManager::ServerConfig::GetGAEServerDomain(const std::string& default_value) {
    std::string result;
    return GetConfig(kConfGAEDomainKey, result) ? result : default_value;
}

inline ResourceManager::RuleConfig& ResourceManager::RuleConfig::operator<<(const std::string& domain) {
    domains_.push_back(domain);
    return *this;
}

inline bool ResourceManager::RuleConfig::RequestProxy(const std::string& host) {
    XINFO << "Checking if host " << host << " needs proxy...";
    for(std::vector<std::string>::iterator it = domains_.begin();
        it != domains_.end(); ++it) {
            XINFO << "Current rule is: " << *it;

            if(host.length() < it->length())
                continue;
            if(host.compare(host.length() - it->length(), it->length(), *it) == 0) {
                XINFO << "Host " << host << " matches rule " << *it << ", need proxy.";
                return true;
            }
    }

    XINFO << "Host " << host << " does not need proxy.";
    return false;
}

#endif // RESOURCE_MANAGER_H
