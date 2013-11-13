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
