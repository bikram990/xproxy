#include "log.h"

namespace xproxy_log {
    ExtraLogAttributes g_attrs;

//    logger& LogExtraInfo(logger& log, const std::string& file, const std::string& function, int line) {
//        g_attrs.set_file_attr(file);
//        g_attrs.set_function_attr(function);
//        g_attrs.set_line_attr(line);
//        return log;
//    }

    void InitLogging() {
        namespace logging = boost::log;
        namespace attrs = boost::log::attributes;
        namespace src = boost::log::sources;
        namespace sinks = boost::log::sinks;
        namespace expr = boost::log::expressions;
        namespace keywords = boost::log::keywords;

        logging::formatter formatter =
            expr::stream << expr::attr<unsigned int>("LineID") << ": "
                         << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S") << " ["
                         << expr::attr<SeverityLevel>("Severity") << "] ["
                         << expr::attr<attrs::current_process_id::value_type>("ProcessID")
                         << ":"
                         << expr::attr<attrs::current_thread_id::value_type>("ThreadID")
                         << "] ["
                         << expr::attr<std::string>("Function")
                         << ":"
                         << expr::attr<std::string >("File")
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
        core->add_global_attribute("File", *g_attrs.get_file_attr());
        core->add_global_attribute("Function", *g_attrs.get_function_attr());
        core->add_global_attribute("Line", *g_attrs.get_line_attr());

        logging::add_common_attributes();
    }

    void Log(logger& log, SeverityLevel level, const char *msg) {
        boost::log::record rec = log.open_record(boost::log::keywords::severity = level);
        if(rec) {
            boost::log::record_ostream stream(rec);
            stream << msg;
            stream.flush();
            log.push_record(boost::move(rec));
        }
    }
}

