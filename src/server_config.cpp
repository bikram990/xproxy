#include <boost/property_tree/ini_parser.hpp>
#include "xproxy/conf/server_config.hpp"

namespace xproxy {
namespace conf {

namespace {
    util::Singleton<ServerConfig> config_;
}

bool ServerConfig::init(const std::string& conf_file) {
    return instance().LoadConfig(conf_file);
}

ServerConfig &ServerConfig::instance() {
    return config_.get();
}

bool ServerConfig::LoadConfig(const std::string& conf_file) {
    try {
        boost::property_tree::ini_parser::read_ini(conf_file, conf_tree_);
    } catch(boost::property_tree::ini_parser::ini_parser_error& e) {
        XFATAL << "Error loading configuration from " << conf_file
               << ", reason: " << e.what();
        return false;
    }
    return true;
}

} // namespace conf
} // namespace xproxy
