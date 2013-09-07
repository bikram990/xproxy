#ifndef LOG_HPP
#define LOG_HPP

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>


class Log : private boost::noncopyable {
public:
    static void set_debug_level(boost::log::trivial::severity_level level) {
        level_ = level;
    }

    static void debug(const char *msg) {
        Log::instance().log(boost::log::trivial::debug, msg);
    }

    static void warn(const char *msg) {
        Log::instance().log(boost::log::trivial::warning, msg);
    }

    static void error(const char *msg) {
        Log::instance().log(boost::log::trivial::error, msg);
    }

private:
    static Log& instance() {
        if(!instance_) {
            // TODO add lock for MSVC, it is not thread-safe
            // gcc is thread-safe
            static Log temp;
            instance_ = &temp;
        }
        return *instance_;
    }

    void log(boost::log::trivial::severity_level level, const char *msg) {
        BOOST_LOG_SEV(log_, level) << msg;
    }

    // Log(boost::log::trivial::severity_level level = boost::log::trivial::debug) {
    Log() {
        boost::log::add_file_log(
            boost::log::keywords::file_name = "xproxy_%N.log",
            boost::log::keywords::rotation_size = 10 * 1024 * 1024,
            boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
            boost::log::keywords::format = "[%TimeStamp%]: %Message%"
        );
        boost::log::core::get()->set_filter(
            boost::log::trivial::severity >= level_
        );
        boost::log::add_common_attributes();
    }

    ~Log() {}

    static boost::log::trivial::severity_level level_;
    static Log *instance_;

    boost::log::sources::severity_logger<boost::log::trivial::severity_level> log_;
};

boost::log::trivial::severity_level Log::level_ = boost::log::trivial::debug;
Log *Log::instance_ = NULL;//nullptr;

#endif // LOG_HPP
