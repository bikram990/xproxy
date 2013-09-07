#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include "log.h"

boost::log::trivial::severity_level Log::level_ = boost::log::trivial::debug;
Log *Log::instance_ = NULL;//nullptr;

Log::Log() {
    boost::log::formatter formatter =
            boost::log::expressions::stream << boost::log::expressions::attr<unsigned int>("LineID") << ": "
                                            << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S") << " *"
                                            << boost::log::expressions::attr<int>("Severity") << "* "
                                            << boost::log::expressions::message;
    boost::log::add_file_log(
                boost::log::keywords::file_name = "xproxy_%N.log",
                boost::log::keywords::rotation_size = 10 * 1024 * 1024,
                boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
                boost::log::keywords::format = "[%TimeStamp%]: %Message%"
            )->set_formatter(formatter);
    boost::log::core::get()->set_filter(
                boost::log::trivial::severity >= level_
                );
    boost::log::add_common_attributes();
}

Log::~Log() {
    // do nothing here
}

void Log::set_debug_level(boost::log::trivial::severity_level level) {
    level_ = level;
}

void Log::debug(const char *msg) {
    Log::instance().log(boost::log::trivial::debug, msg);
}

void Log::warn(const char *msg) {
    Log::instance().log(boost::log::trivial::warning, msg);
}

void Log::error(const char *msg) {
    Log::instance().log(boost::log::trivial::error, msg);
}

Log& Log::instance() {
    if(!instance_) {
        // TODO add lock for MSVC, it is not thread-safe
        // gcc is thread-safe
        static Log temp;
        instance_ = &temp;
    }
    return *instance_;
}

void Log::log(boost::log::trivial::severity_level level, const char *msg) {
    BOOST_LOG_SEV(log_, level) << msg;
}
