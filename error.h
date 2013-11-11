#ifndef ERROR_H
#define ERROR_H

#include <boost/system/error_code.hpp>

#define GENERIC_ERROR  1

class ErrorCategory : public boost::system::error_category {
public:
    enum Errors {
        kGenericError = GENERIC_ERROR
    };

    const char *name() const {
        return "xproxy.error";
    }

    std::string message(int value) const {
        if(value == kGenericError)
            return "Generic error";

        return "xproxy error";
    }
};

const boost::system::error_category& GetErrorCategory() {
    static ErrorCategory instance;
    return instance;
}

static const boost::system::error_category& ErrorCategory = GetErrorCategory();

namespace boost {
namespace system {

template<> struct is_error_code_enum<ErrorCategory::Errors> {
    static const bool value = true;
};

} // namespace system
} // namespace boost

inline boost::system::error_code make_error_code(ErrorCategory::Errors e) {
    return boost::system::error_code(static_cast<int>(e), GetErrorCategory());
}

#endif // ERROR_H
