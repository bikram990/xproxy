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

#define CHECK_RETURN(error_code, error_type) \
    do {\
        if (error_code) {\
            XERROR_WITH_ID(this) << error_type << " error, code: " << error_code.value()\
                                 << ", message: " << error_code.message();\
            stop();\
            return;\
        }\
    } while(0)

#endif // COMMON_HPP
