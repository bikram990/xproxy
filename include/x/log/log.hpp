#ifndef LOG_HPP
#define LOG_HPP

#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>

namespace x {
namespace log {

static log4cpp::Category& logger = log4cpp::Category::getRoot();

inline void init_logging(const std::string& conf_file = "xproxy.conf") {
    log4cpp::PropertyConfigurator::configure(conf_file);
}

inline bool debug_enabled() {
    return logger.isDebugEnabled();
}

} // namespace log
} // namespace x


#define META_INFO "[" << __FILE__ << ":" << __FUNCTION__ << "():L" << __LINE__ << "] "

#define XDEBUG x::log::logger << log4cpp::Priority::DEBUG << META_INFO
#define XINFO  x::log::logger << log4cpp::Priority::INFO  << META_INFO
#define XWARN  x::log::logger << log4cpp::Priority::WARN  << META_INFO
#define XERROR x::log::logger << log4cpp::Priority::ERROR << META_INFO
#define XFATAL x::log::logger << log4cpp::Priority::FATAL << META_INFO

#define XDEBUG_WITH_ID(cls) x::log::logger << log4cpp::Priority::DEBUG << META_INFO << "[id: " << cls->id() << "] "
#define XINFO_WITH_ID(cls)  x::log::logger << log4cpp::Priority::INFO  << META_INFO << "[id: " << cls->id() << "] "
#define XWARN_WITH_ID(cls)  x::log::logger << log4cpp::Priority::WARN  << META_INFO << "[id: " << cls->id() << "] "
#define XERROR_WITH_ID(cls) x::log::logger << log4cpp::Priority::ERROR << META_INFO << "[id: " << cls->id() << "] "
#define XFATAL_WITH_ID(cls) x::log::logger << log4cpp::Priority::FATAL << META_INFO << "[id: " << cls->id() << "] "

#endif // LOG_HPP
