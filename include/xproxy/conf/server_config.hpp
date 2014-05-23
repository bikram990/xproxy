#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <boost/property_tree/ptree.hpp>
#include "xproxy/common.hpp"
#include "xproxy/log/log.hpp"
#include "xproxy/util/singleton.hpp"

namespace xproxy {
namespace conf {

class ServerConfig {
    friend class util::Singleton<ServerConfig>;
public:
    static bool init(const std::string& conf_file = "xproxy.conf");
    static ServerConfig& instance();

    template<typename TypeT>
    bool getConfig(const std::string& path, TypeT& return_value) {
        try {
            return_value = conf_tree_.get<TypeT>(path);
        } catch(boost::property_tree::ptree_error& e) {
            XWARN << "Error reading config, path: " << path
                  << ", reason: " << e.what();
            return false;
        }
        return true;
    }

    DEFAULT_DTOR(ServerConfig);

private:
    ServerConfig() = default;

    bool loadConfig(const std::string& conf_file);

private:
    boost::property_tree::ptree conf_tree_;
};

} // namespace conf
} // namespace xproxy

#endif // SERVER_CONFIG_HPP
