#ifndef LOG_H
#define LOG_H

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>


class Log : private boost::noncopyable {
public:
    static void set_debug_level(boost::log::trivial::severity_level level);
    static void debug(const char *msg);
    static void warn(const char *msg);
    static void error(const char *msg);

private:
    static Log& instance();

    void log(boost::log::trivial::severity_level level, const char *msg);

    Log();
    ~Log();

    static boost::log::trivial::severity_level level_;
    static Log *instance_;

    boost::log::sources::severity_logger<boost::log::trivial::severity_level> log_;
};

#endif // LOG_H
