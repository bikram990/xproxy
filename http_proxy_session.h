#ifndef HTTP_PROXY_SESSION_H
#define HTTP_PROXY_SESSION_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>
#include "http_proxy_request.h"

class HttpProxySessionManager;


class HttpProxySession
        : public boost::enable_shared_from_this<HttpProxySession>,
          private boost::noncopyable {
public:
    typedef HttpProxyRequest::BuildResult ResultType;

    HttpProxySession(boost::asio::io_service& service,
                     HttpProxySessionManager& manager);
    ~HttpProxySession();
    boost::asio::ip::tcp::socket& LocalSocket();
    boost::asio::ip::tcp::socket& RemoteSocket();
    void Start();
    void Stop();

private:
    void HandleLocalRead(const boost::system::error_code& e, std::size_t size);
    void HandleLocalWrite(const boost::system::error_code& e, bool finished);

    void ResolveRemote();
    void HandleConnect(const boost::system::error_code& e);
    void HandleRemoteWrite(const boost::system::error_code& e);
    void HandleRemoteReadStatusLine(const boost::system::error_code& e);
    void HandleRemoteReadHeaders(const boost::system::error_code& e);
    void HandleRemoteReadBody(const boost::system::error_code& e);

    boost::asio::ip::tcp::socket local_socket_;
    boost::asio::ip::tcp::socket remote_socket_;
    boost::asio::ip::tcp::resolver resolver_; // to resolve remote address
    HttpProxySessionManager& manager_;

    HttpProxyRequest request_;

    //boost::array<char, 4096> buffer_;
    boost::array<char, 4096> local_buffer_;
    //boost::array<char, 4096> remote_buffer_;
    boost::asio::streambuf remote_buffer_;
};

typedef boost::shared_ptr<HttpProxySession> HttpProxySessionPtr;

#endif // HTTP_PROXY_SESSION_H
