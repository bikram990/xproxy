#include <boost/property_tree/ini_parser.hpp>
#include "log.h"
#include "proxy_configuration.h"

ProxyConfiguration::ProxyConfiguration(const std::string& conf_file)
    : conf_file_(conf_file) {
    TRACE_THIS_PTR;
    // TODO handle the return value here
    init();
}

ProxyConfiguration::~ProxyConfiguration() {
    TRACE_THIS_PTR;
}

bool ProxyConfiguration::init() {
    // TOOD handle exception here
    boost::property_tree::ini_parser::read_ini(conf_file_, conf_tree_);
    return true;
}

short ProxyConfiguration::GetProxyPort(short default_value) {
    // TODO implement this
    return default_value;
}
