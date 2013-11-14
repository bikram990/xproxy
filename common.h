#ifndef COMMON_H
#define COMMON_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#define DECLARE_GENERAL_CONSTRUCTOR(Class) \
    Class() { TRACE_THIS_PTR; }

#define DECLARE_GENERAL_DESTRUCTOR(Class) \
    virtual ~Class() { TRACE_THIS_PTR; }

#define CLEAN_MEMBER(member) \
    if(member) { \
        delete member; \
        member = NULL; \
    }

#define CLEAN_ARRAY_MEMBER(member) \
    if(member) { \
        delete [] member; \
        member = NULL; \
    }

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
