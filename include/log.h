#ifndef LOG_H
#define LOG_H

#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>

namespace xproxy { namespace log {

static log4cpp::Category& logger = log4cpp::Category::getRoot();

void InitLogging(const std::string& conf_file = "xproxy.conf");

}}


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

#define XDEBUG_X if (xproxy::log::logger.isDebugEnabled()) \
    xproxy::log::logger << log4cpp::Priority::DEBUG << META_INFO

#define XDEBUG_WITH_ID_X if (xproxy::log::logger.isDebugEnabled()) \
    xproxy::log::logger << log4cpp::Priority::DEBUG << META_INFO << "[id: " << id() << "] "


#endif // LOG_H
