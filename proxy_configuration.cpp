#include <boost/property_tree/ini_parser.hpp>
#include "proxy_configuration.h"

const std::string ProxyConfiguration::kConfPortKey("basic.port");
const std::string ProxyConfiguration::kConfGAEAppIdKey("proxy_gae.app_id");
const std::string ProxyConfiguration::kConfGAEDomainKey("proxy_gae.domain");

ProxyConfiguration::ProxyConfiguration(const std::string& conf_file)
    : conf_file_(conf_file) {
    TRACE_THIS_PTR;
}

ProxyConfiguration::~ProxyConfiguration() {
    TRACE_THIS_PTR;
}

bool ProxyConfiguration::LoadConfig() {
    try {
        boost::property_tree::ini_parser::read_ini(conf_file_, conf_tree_);
    } catch(boost::property_tree::ini_parser::ini_parser_error& e) {
        XFATAL << "Failed to load configuration from config file: " << conf_file_
               << ", reason: " << e.what();
        return false;
    }
    return true;
}

bool ProxyConfiguration::LoadConfig(const std::string& conf_file) {
    if(conf_file_ != conf_file)
        conf_file_ = conf_file;
    return LoadConfig();
}

short ProxyConfiguration::GetProxyPort(short default_value) {
    short result;
    return GetConfig(kConfPortKey, result) ? result : default_value;
}

const std::string ProxyConfiguration::GetGAEAppId() {
    std::string result;
    return GetConfig(kConfGAEAppIdKey, result) ? result : "";
}

const std::string ProxyConfiguration::GetGAEServerDomain(const std::string& default_value) {
    std::string result;
    return GetConfig(kConfGAEDomainKey, result) ? result : default_value;
}
