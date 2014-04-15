#ifndef COMMON_H
#define COMMON_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>


#define DEFAULT_CTOR_AND_DTOR(T) T() = default; \
    virtual ~T() = default

#define DISABLE_COPY_AND_ASSIGNMENT(T) T(const T&) = delete; \
    T& operator=(const T&) = delete

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

typedef boost::asio::ip::tcp::socket socket_type;
typedef boost::asio::ssl::stream<socket_type> ssl_socket_type;
typedef boost::asio::ssl::stream<socket_type&> ssl_socket_ref_type;
typedef boost::asio::ssl::context ssl_context_type;

enum Protocol {
    kHttp, kHttps
};

enum SSLMode {
    kClient, kServer
};

#endif // COMMON_H
