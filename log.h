#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <fstream>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/log/expressions/formatters.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/empty_deleter.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>


enum severity_level {
    debug = -1,
    info = 0,
    warning,
    error,
    fatal
};

template< typename CharT, typename TraitsT >
    inline std::basic_ostream< CharT, TraitsT >& operator<<
    (std::basic_ostream< CharT, TraitsT >& strm, severity_level level) {
    static const char* const str[] = {
        "debug",
        "info",
        "warning",
        "error",
        "fatal"
    };
    if(level >= debug && level <= fatal) {
        strm << str[level - debug];
    } else {
        strm << static_cast<int>(level);
    }
    return strm;
}


namespace mylog {
    //typedef boost::log::attributes::mutable_constant<int, boost::mutex, boost::lock_guard< boost::mutex > > mutable_int_mt;
    typedef boost::log::attributes::mutable_constant<int> mutable_int_mt;
    //typedef boost::log::attributes::mutable_constant<std::string, boost::mutex, boost::lock_guard< boost::mutex > > mutable_string_mt;
    typedef boost::log::attributes::mutable_constant<std::string> mutable_string_mt;
    static boost::shared_ptr<mylog::mutable_string_mt> file_attr(new mylog::mutable_string_mt(""));
    static boost::shared_ptr<mylog::mutable_string_mt> function_attr(new mylog::mutable_string_mt(""));
    static boost::shared_ptr<mylog::mutable_int_mt> line_attr(new mylog::mutable_int_mt(-1));

    typedef boost::log::sources::severity_logger<severity_level> logger;

    template<typename T>
    static inline T& tag_log_location
        (T &log_,
         const std::string file_,
         int line_,
         const std::string function_) {
        mylog::file_attr->set(file_);
        mylog::function_attr->set(function_);
        mylog::line_attr->set(line_);
        return log_;
    }

    static void init_logging() {
        namespace logging = boost::log;
        namespace attrs = boost::log::attributes;
        namespace src = boost::log::sources;
        namespace sinks = boost::log::sinks;
        namespace expr = boost::log::expressions;
        //namespace fmt = boost::log::formatters;
        namespace keywords = boost::log::keywords;

        boost::shared_ptr< logging::core > core = logging::core::get();

        // Create a backend and attach a couple of streams to it
        boost::shared_ptr< sinks::text_ostream_backend > backend =
            boost::make_shared< sinks::text_ostream_backend >();
        //backend->add_stream(boost::shared_ptr< std::ostream >(&std::clog, logging::empty_deleter()));
        backend->add_stream(boost::shared_ptr<std::ostream>(new std::ofstream("test.log")));

        logging::formatter formatter =
            expr::stream << expr::attr<unsigned int>("LineID") << ": "
                         << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S") << " *"
                         << expr::attr<severity_level>("Severity") << "* "
                         << expr::attr<attrs::current_process_id::value_type>("ProcessID")
                         << ":"
                         << expr::attr<attrs::current_thread_id::value_type>("ThreadID")
                         << " "
                         << expr::attr<std::string>("Function")
                         << ":"
                         << expr::attr<std::string >("File")
                         << ":"
                         << expr::attr< int >("Line")
                         << " "
                         << expr::message;

        // Enable auto-flushing after each log record written
        backend->auto_flush(true);

        // Wrap it into the frontend and register in the core.
        // The backend requires synchronization in the frontend.
        typedef sinks::synchronous_sink< sinks::text_ostream_backend > sink_t;
        boost::shared_ptr< sink_t > sink(new sink_t(backend));
        sink->set_formatter(formatter);
        core->add_sink(sink);

        core->add_global_attribute("File", *mylog::file_attr);
        core->add_global_attribute("Function", *mylog::function_attr);
        core->add_global_attribute("Line", *mylog::line_attr);
        logging::add_common_attributes();
    }
}


#define LOG(log_, lvl_) BOOST_LOG_SEV(mylog::tag_log_location(log_, __FILE__, __LINE__, __FUNCTION__), lvl_)


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
