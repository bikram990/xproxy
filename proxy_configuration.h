#ifndef PROXY_CONFIGURATION_H
#define PROXY_CONFIGURATION_H

#include <string>
#include <boost/property_tree/ptree.hpp>
#include "log.h"


class ProxyConfiguration {
public:
    const static std::string kConfPortKey;
    const static std::string kConfGAEAppIdKey;
    const static std::string kConfGAEDomainKey;

    ProxyConfiguration(const std::string& conf_file = "xproxy.conf");
    ~ProxyConfiguration();

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
    std::string conf_file_;
    boost::property_tree::ptree conf_tree_;
};

#endif // PROXY_CONFIGURATION_H
