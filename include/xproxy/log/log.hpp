#ifndef LOG_HPP
#define LOG_HPP

#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>

namespace xproxy {
namespace log {

static log4cpp::Category& logger = log4cpp::Category::getRoot();

inline void initLogging(const std::string& conf_file = "xproxy.conf") {
    log4cpp::PropertyConfigurator::configure(conf_file);
}

} // namespace log
} // namespace xproxy


#define META_INFO "[" << __FILE__ << ":" << __FUNCTION__ << "():L" << __LINE__ << "] "

#define XDEBUG xproxy::log::logger << log4cpp::Priority::DEBUG << META_INFO
#define XINFO  xproxy::log::logger << log4cpp::Priority::INFO  << META_INFO
#define XWARN  xproxy::log::logger << log4cpp::Priority::WARN  << META_INFO
#define XERROR xproxy::log::logger << log4cpp::Priority::ERROR << META_INFO
#define XFATAL xproxy::log::logger << log4cpp::Priority::FATAL << META_INFO

#define XDEBUG_WITH_ID xproxy::log::logger << log4cpp::Priority::DEBUG << META_INFO << "[id: " << id() << "] "
#define XINFO_WITH_ID  xproxy::log::logger << log4cpp::Priority::INFO  << META_INFO << "[id: " << id() << "] "
#define XWARN_WITH_ID  xproxy::log::logger << log4cpp::Priority::WARN  << META_INFO << "[id: " << id() << "] "
#define XERROR_WITH_ID xproxy::log::logger << log4cpp::Priority::ERROR << META_INFO << "[id: " << id() << "] "
#define XFATAL_WITH_ID xproxy::log::logger << log4cpp::Priority::FATAL << META_INFO << "[id: " << id() << "] "

#define XDEBUG_ID_WITH(klass) xproxy::log::logger << log4cpp::Priority::DEBUG << META_INFO << "[id: " << klass.id() << "] "
#define XINFO_ID_WITH(klass)  xproxy::log::logger << log4cpp::Priority::INFO  << META_INFO << "[id: " << klass.id() << "] "
#define XWARN_ID_WITH(klass)  xproxy::log::logger << log4cpp::Priority::WARN  << META_INFO << "[id: " << klass.id() << "] "
#define XERROR_ID_WITH(klass) xproxy::log::logger << log4cpp::Priority::ERROR << META_INFO << "[id: " << klass.id() << "] "
#define XFATAL_ID_WITH(klass) xproxy::log::logger << log4cpp::Priority::FATAL << META_INFO << "[id: " << klass.id() << "] "

#define XDEBUG_X if (xproxy::log::logger.isDebugEnabled()) \
    xproxy::log::logger << log4cpp::Priority::DEBUG << META_INFO

#define XDEBUG_WITH_ID_X if (xproxy::log::logger.isDebugEnabled()) \
    xproxy::log::logger << log4cpp::Priority::DEBUG << META_INFO << "[id: " << id() << "] "


#endif // LOG_HPP
