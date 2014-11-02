#ifndef CONFIG_HPP
#define CONFIG_HPP

namespace x {
namespace conf {

class config {
public:
    config(const std::string& conf_file = "xproxy.conf") {
        if(!load_config(conf_file)) {
            #warning throw an exception here?
        }
    }

    DEFAULT_DTOR(config);

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
    bool load_config(const std::string& conf_file) {
        try {
            boost::property_tree::ini_parser::read_ini(conf_file, conf_tree_);
        } catch(boost::property_tree::ini_parser::ini_parser_error& e) {
            XFATAL << "load config error, path: " << conf_file
                   << ", reason: " << e.what();
            return false;
        }

        XINFO << "config file " << conf_file << " loaded.";
        return true;
    }

    boost::property_tree::ptree conf_tree_;
};

} // namespace conf
} // namespace x

#endif // CONFIG_HPP
