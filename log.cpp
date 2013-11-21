#include <fstream>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/empty_deleter.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include "log.h"

namespace xproxy {
namespace log {

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", SeverityLevel)

AttrsPtr g_attrs;
LoggerPtr g_logger;

void InitLogging() {
    g_attrs = boost::make_shared<ExtraLogAttributes>();
    g_logger = boost::make_shared<Logger>();

    namespace logging = boost::log;
    namespace attrs = boost::log::attributes;
    namespace sinks = boost::log::sinks;
    namespace expr = boost::log::expressions;

    logging::formatter formatter =
        expr::stream << "[" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S") << "] ["
                     << severity << "] ["
                     << expr::attr<attrs::current_process_id::value_type>("ProcessID")
                     << ":"
                     << expr::attr<attrs::current_thread_id::value_type>("ThreadID")
                     << "] ["
                     << expr::attr<std::string>("File")
                     << ":"
                     << expr::attr<std::string >("Function") << "()"
                     << ":"
                     << expr::attr< int >("Line")
                     << "] "
                     << expr::message;

    typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;
    boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();
    sink->locked_backend()->add_stream(boost::make_shared<std::ofstream>("xproxy.log"));
    boost::shared_ptr<std::ostream> console(&std::clog, logging::empty_deleter());
    sink->locked_backend()->add_stream(console);
    sink->set_formatter(formatter);

    boost::shared_ptr<logging::core> core = logging::core::get();
    core->add_sink(sink);
    core->add_global_attribute("File", *g_attrs->file_attr);
    core->add_global_attribute("Function", *g_attrs->function_attr);
    core->add_global_attribute("Line", *g_attrs->line_attr);

    logging::add_common_attributes();
}

void SetLogLevel(const std::string& level) {
    static const std::map<std::string, SeverityLevel> map = {
        {"trace", kTrace}, {"debug", kDebug}, {"info", kInfo},
        {"warning", kWarning}, {"error", kError}, {"fatal", kFatal}
    };
    std::map<std::string, SeverityLevel>::const_iterator it = map.find(level);
    if(it != map.end())
        boost::log::core::get()->set_filter(severity >= it->second);
    else
        boost::log::core::get()->set_filter(severity >= kInfo);
}
}
}
