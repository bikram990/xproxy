#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "x/common.hpp"
#include "x/log/log.hpp"

namespace x {
namespace conf {

class config {
public:
    config(const std::string& conf_file = "xproxy.conf")
        : conf_file_(conf_file) {}

    DEFAULT_DTOR(config);

    bool load_config() {
        try {
            boost::property_tree::ini_parser::read_ini(conf_file_, conf_tree_);
        } catch(boost::property_tree::ini_parser::ini_parser_error& e) {
            XFATAL << "load config error, path: " << conf_file_
                   << ", reason: " << e.what();
            return false;
        }

        XINFO << "config file " << conf_file_ << " loaded.";
        return true;
    }

    template<typename T>
    bool get_config(const std::string& path, T& return_value) {
        try {
            return_value = conf_tree_.get<T>(path);
        } catch(boost::property_tree::ptree_error& e) {
            XWARN << "read config error, path: " << path
                  << ", reason: " << e.what();
            return false;
        }
        return true;
    }

private:
    std::string conf_file_;
    boost::property_tree::ptree conf_tree_;
};

} // namespace conf
} // namespace x

#endif // CONFIG_HPP
