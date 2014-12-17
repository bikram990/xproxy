#ifndef COMMON_HPP
#define COMMON_HPP

#define DEFAULT_CTOR_AND_DTOR(T) T() = default; \
    virtual ~T() = default

#define DEFAULT_CTOR(T) T() = default

#define DEFAULT_DTOR(T) virtual ~T() = default

#define MAKE_NONCOPYABLE(T) T(const T&) = delete; \
    T& operator=(const T&) = delete

/*
 * Some handy definitions for HTTP message processing.
 */
#define CR '\r'
#define LF '\n'
#define CRLF "\r\n"
#define END_CHUNK "0\r\n\r\n"

/*
 * Short read error in SSL mode.
 *
 * The actual code is 335544539, for how is the code generated, see function
 * engine::map_error_code(...) in asio/ssl/detail/impl/engine.ipp.
 *
 * When we get short read error, we should treat it as EOF in normal mode.
 */
#define SSL_SHORT_READ(ec) ((ec.value() & 0xFF) == SSL_R_SHORT_READ)

#define CHECK_LOG_EXEC_RETURN(error_code, error_type, func) \
    do {\
        if (error_code) {\
            XERROR_WITH_ID(this) << error_type << " error, code: " << error_code.value()\
                                 << ", message: " << error_code.message();\
            func();\
            return;\
        }\
    } while (0)

#warning add stack trace here
#define ASSERT(exp)\
    do {\
        XFATAL << "assertion error: " << #exp\
               << ", line: " << __LINE__\
               << ", function: " << __FUNCTION__;\
        assert((exp) && #exp);\
    } while (0)

#define ASSERT_RETVAL(exp, ret)\
    do {\
        if (exp) break;\
        ASSERT(exp);\
        return ret;\
    } while (0)

#define ASSERT_RETNONE(exp)\
    do {\
        if (exp) break;\
        ASSERT(exp);\
        return;\
    } while (0)

#define ASSERT_NOTHING(exp)\
    do {\
        if (exp) break;\
        ASSERT(exp);\
    } while (0)

#define ASSERT_EXEC_RETNONE(exp, func)\
    do {\
        if (exp) break;\
        ASSERT(exp);\
        func();\
        return;\
    } while (0)

#endif // COMMON_HPP
