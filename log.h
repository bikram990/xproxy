#ifndef LOG_H
#define LOG_H

#include <boost/make_shared.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/log/expressions/attr.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>


namespace xproxy {
namespace log {

enum SeverityLevel {
    kTrace = 0,
    kDebug,
    kInfo,
    kWarning,
    kError,
    kFatal
};

struct ExtraLogAttributes {
    //typedef boost::log::attributes::mutable_constant<int, boost::mutex, boost::lock_guard< boost::mutex > > MutableIntMt;
    typedef boost::log::attributes::mutable_constant<int> MutableIntMt;
    //typedef boost::log::attributes::mutable_constant<std::string, boost::mutex, boost::lock_guard< boost::mutex > > MutableStringMt;
    typedef boost::log::attributes::mutable_constant<std::string> MutableStringMt;
    typedef boost::shared_ptr<MutableIntMt> MutableIntMtPtr;
    typedef boost::shared_ptr<MutableStringMt> MutableStringMtPtr;

    MutableStringMtPtr file_attr;
    MutableStringMtPtr function_attr;
    MutableIntMtPtr line_attr;

    ExtraLogAttributes()
        : file_attr(boost::make_shared<MutableStringMt>("")),
          function_attr(boost::make_shared<MutableStringMt>("")),
          line_attr(boost::make_shared<MutableIntMt>(0)) {}
};

typedef boost::log::sources::severity_logger<SeverityLevel> Logger;
typedef boost::shared_ptr<Logger> LoggerPtr;
typedef boost::shared_ptr<ExtraLogAttributes> AttrsPtr;

extern AttrsPtr g_attrs;
extern LoggerPtr g_logger;

template<typename LoggerT>
inline LoggerT& LogExtraInfo(LoggerT& log, const std::string& file,
                             const std::string& function, int line) {
    g_attrs->file_attr->set(file);
    g_attrs->function_attr->set(function);
    g_attrs->line_attr->set(line);
    return log;
}

template<typename CharT, typename TraitsT>
inline std::basic_ostream<CharT, TraitsT>&
operator<<(std::basic_ostream<CharT, TraitsT>& stream, SeverityLevel level) {
    static const char* const str[] = {
        "TRACE", "DEBUG", " INFO", " WARN", "ERROR", "FATAL"
    };
    if(level >= kTrace && level <= kFatal)
        stream << str[level - kTrace];
    else
        stream << static_cast<int>(level);
    return stream;
}

void InitLogging();

}
}


#define XLOG(logger, level) BOOST_LOG_SEV(xproxy::log::LogExtraInfo((logger), __FILE__, __FUNCTION__, __LINE__), (level))

#define XTRACE XLOG(*xproxy::log::g_logger, xproxy::log::kTrace)
#define XDEBUG XLOG(*xproxy::log::g_logger, xproxy::log::kDebug)
#define XINFO  XLOG(*xproxy::log::g_logger, xproxy::log::kInfo)
#define XWARN  XLOG(*xproxy::log::g_logger, xproxy::log::kWarning)
#define XERROR XLOG(*xproxy::log::g_logger, xproxy::log::kError)
#define XFATAL XLOG(*xproxy::log::g_logger, xproxy::log::kFatal)

#define XTRACE_WITH_ID XLOG(*xproxy::log::g_logger, xproxy::log::kTrace)   << "[id: " << id_ << "] "
#define XDEBUG_WITH_ID XLOG(*xproxy::log::g_logger, xproxy::log::kDebug)   << "[id: " << id_ << "] "
#define XINFO_WITH_ID  XLOG(*xproxy::log::g_logger, xproxy::log::kInfo)    << "[id: " << id_ << "] "
#define XWARN_WITH_ID  XLOG(*xproxy::log::g_logger, xproxy::log::kWarning) << "[id: " << id_ << "] "
#define XERROR_WITH_ID XLOG(*xproxy::log::g_logger, xproxy::log::kError)   << "[id: " << id_ << "] "
#define XFATAL_WITH_ID XLOG(*xproxy::log::g_logger, xproxy::log::kFatal)   << "[id: " << id_ << "] "

#define TRACE_THIS_PTR XTRACE << "Address of this pointer: [" << this << "]."

#define TRACE_THIS_PTR_WITH_ID XTRACE << "Address of this pointer: [" << this << "], id: [" << id_ << "]."

#endif // LOG_H
