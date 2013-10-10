#ifndef HTTPS_DIRECT_HANDLER_H
#define HTTPS_DIRECT_HANDLER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "http_request.h"
#include "http_response.h"
#include "request_handler.h"


class HttpProxySession;

class HttpsDirectHandler : public RequestHandler {
public:
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
    HttpsDirectHandler(HttpProxySession& session, HttpRequestPtr request);
    ~HttpsDirectHandler();

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

    bool VerifyCertificate(bool pre_verified, boost::asio::ssl::verify_context& ctx);
    void OnRemoteHandshaken(const boost::system::error_code& e);

    HttpProxySession& session_;

    boost::asio::ip::tcp::socket& local_socket_;
    boost::asio::ssl::context context_;
    ssl_socket remote_socket_; // TODO close the socket?
    boost::asio::ip::tcp::resolver resolver_;

    boost::asio::streambuf remote_buffer_;

    HttpRequestPtr request_;
    HttpResponse response_;
};

#endif // HTTPS_DIRECT_HANDLER_H
