#ifndef HTTP_DIRECT_HANDLER_H
#define HTTP_DIRECT_HANDLER_H

#include <boost/asio.hpp>
#include "http_request.h"
#include "http_response.h"
#include "request_handler.h"


class HttpProxySession;

class HttpDirectHandler : public RequestHandler {
public:
    HttpDirectHandler(HttpProxySession& session,
                      boost::asio::io_service& service,
                      boost::asio::ip::tcp::socket& local_socket,
                      boost::asio::ip::tcp::socket& remote_socket,
                      HttpRequestPtr request);
    ~HttpDirectHandler();

    void HandleRequest();

private:
    void ResolveRemote();
    void OnRemoteConnected(const boost::system::error_code& e);
    void OnRemoteDataSent(const boost::system::error_code& e);
    void OnRemoteStatusLineReceived(const boost::system::error_code& e);
    void OnRemoteHeadersReceived(const boost::system::error_code& e);
    void OnRemoteBodyReceived(const boost::system::error_code& e);
    void OnRemoteChunksReceived(const boost::system::error_code& e);
    void OnLocalDataSent(const boost::system::error_code& e, bool finished);

    HttpProxySession& session_;

    boost::asio::ip::tcp::socket& local_socket_;
    boost::asio::ip::tcp::socket& remote_socket_;
    boost::asio::ip::tcp::resolver resolver_;

    boost::asio::streambuf remote_buffer_;

    HttpRequestPtr request_;
    HttpResponse response_;
};

#endif // HTTP_DIRECT_HANDLER_H
