#ifndef LOG_H
#define LOG_H

#include <fstream>
#include <boost/noncopyable.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/attr.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/empty_deleter.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>


namespace xproxy_log {

    enum SeverityLevel {
        kDebug = 0,
        kInfo,
        kWarning,
        kError,
        kFatal
    };

    class ExtraLogAttributes {
    public:
        //typedef boost::log::attributes::mutable_constant<int, boost::mutex, boost::lock_guard< boost::mutex > > MutableIntMt;
        typedef boost::log::attributes::mutable_constant<int> MutableIntMt;
        //typedef boost::log::attributes::mutable_constant<std::string, boost::mutex, boost::lock_guard< boost::mutex > > MutableStringMt;
        typedef boost::log::attributes::mutable_constant<std::string> MutableStringMt;

        ExtraLogAttributes() {
            init();
        }

        boost::shared_ptr<MutableStringMt> get_file_attr() const {
            return file_attr_;
        }

        void set_file_attr(const std::string& file) {
            file_attr_->set(file);
        }

        boost::shared_ptr<MutableStringMt> get_function_attr() const {
            return function_attr_;
        }

        void set_function_attr(const std::string& function) {
            function_attr_->set(function);
        }

        boost::shared_ptr<MutableIntMt> get_line_attr() const {
            return line_attr_;
        }

        void set_line_attr(int line) {
            line_attr_->set(line);
        }

    private:
        void init() {
            if(inited_) return;

            file_attr_ = boost::make_shared<MutableStringMt>("");
            function_attr_ = boost::make_shared<MutableStringMt>("");
            line_attr_ = boost::make_shared<MutableIntMt>(0);

            inited_ = true;
        }

        boost::shared_ptr<MutableStringMt> file_attr_;
        boost::shared_ptr<MutableStringMt> function_attr_;
        boost::shared_ptr<MutableIntMt> line_attr_;

        bool inited_;
    };

    typedef boost::log::sources::severity_logger<SeverityLevel> Logger;

    extern ExtraLogAttributes g_attrs;

    template<typename LoggerT>
    inline LoggerT& LogExtraInfo(LoggerT& log, const std::string& file, const std::string& function, int line) {
        g_attrs.set_file_attr(file);
        g_attrs.set_function_attr(function);
        g_attrs.set_line_attr(line);
        return LoggerT;
    }

    void InitLogging();

    template<typename LoggerT>
    void Log(LoggerT& log, SeverityLevel level, const char *msg);
}

template<typename CharT, typename TraitsT>
inline std::basic_ostream<CharT, TraitsT>& operator<<(
        std::basic_ostream<CharT, TraitsT>& stream,
        xproxy_log::SeverityLevel level) {
    std::cout << "in func <<" << std::endl;
    static const char* const str[] = {
        "debug", "info", "warning", "error", "fatal"
    };
    if(level >= xproxy_log::kDebug && level <= xproxy_log::kFatal) {
        stream << str[level - xproxy_log::kDebug];
    } else {
        stream << static_cast<int>(level);
    }
    return stream;
}

template< typename CharT, typename TraitsT, typename AllocatorT >
inline boost::log::basic_formatting_ostream<CharT, TraitsT, AllocatorT>& operator<<(
        boost::log::basic_formatting_ostream<CharT, TraitsT, AllocatorT>& stream,
        xproxy_log::SeverityLevel level) {
    std::cout << "in func <<" << std::endl;
    static const char* const str[] = {
        "debug", "info", "warning", "error", "fatal"
    };
    if(level >= xproxy_log::kDebug && level <= xproxy_log::kFatal) {
        stream << str[level - xproxy_log::kDebug];
    } else {
        stream << static_cast<int>(level);
    }
    return stream;
}

//#define LOG(log, level, msg) xproxy_log::Log(xproxy_log::LogExtraInfo(log, __FILE__, __FUNCTION__, __LINE__), level, msg)
#define LOG(log, level, msg) BOOST_LOG_SEV(xproxy_log::LogExtraInfo(log, __FILE__, __FUNCTION__, __LINE__), level) << msg

#define DEBUG(log, msg) LOG(log, xproxy_log::kDebug, msg)
#define ERROR(log, msg) LOG(log, xproxy_log::kError, msg)

//#define LOG(log_, lvl_) BOOTS_LOG_SEV(mylog::tag_log_location(log_, __FILE__, __LINE__, __FUNCTION__), lvl_)

#endif // LOG_H
