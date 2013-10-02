#ifndef PROXY_CONFIGURATION_H
#define PROXY_CONFIGURATION_H

#include <string>
#include <boost/property_tree/ptree.hpp>


class ProxyConfiguration {
public:
    ProxyConfiguration(const std::string& conf_file = "xproxy.conf");
    ~ProxyConfiguration();

    bool LoadConfig();
    bool LoadConfig(const std::string& conf_file);

    short GetProxyPort(short default_value = 7077);

    template<typename TypeT>
    TypeT GetConfig(char *path) {
        // TODO handle exception here
        return conf_tree_.get<TypeT>(path);
    }

private:
    std::string conf_file_;
    boost::property_tree::ptree conf_tree_;
};

#endif // PROXY_CONFIGURATION_H
