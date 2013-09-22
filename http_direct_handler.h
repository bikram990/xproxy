#ifndef HTTP_DIRECT_HANDLER_H
#define HTTP_DIRECT_HANDLER_H

#include <boost/asio.hpp>
#include "http_proxy_request.h"
#include "http_proxy_response.h"


class HttpProxySession;

class HttpDirectHandler {
public:
    typedef HttpProxyRequest::BuildResult ResultType;

    HttpDirectHandler(HttpProxySession& session,
                      boost::asio::io_service& service,
                      boost::asio::ip::tcp::socket& local_socket,
                      boost::asio::ip::tcp::socket& remote_socket);
    ~HttpDirectHandler();

    void HandleRequest(char *data, std::size_t size);

private:
    void ResolveRemote();
    void HandleConnect(const boost::system::error_code& e);
    void HandleRemoteWrite(const boost::system::error_code& e);
    void HandleRemoteReadStatusLine(const boost::system::error_code& e);
    void HandleRemoteReadHeaders(const boost::system::error_code& e);
    void HandleRemoteReadBody(const boost::system::error_code& e);
    void HandleLocalWrite(const boost::system::error_code& e, bool finished);

    HttpProxySession& session_;

    boost::asio::ip::tcp::socket& local_socket_;
    boost::asio::ip::tcp::socket& remote_socket_;
    boost::asio::ip::tcp::resolver resolver_;

    boost::asio::streambuf remote_buffer_;

    HttpProxyRequest request_;
    HttpProxyResponse response_;
};

#endif // HTTP_DIRECT_HANDLER_H
