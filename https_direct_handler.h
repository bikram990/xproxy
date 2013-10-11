#ifndef HTTPS_DIRECT_HANDLER_H
#define HTTPS_DIRECT_HANDLER_H

#include <vector>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "http_request.h"
#include "http_response.h"
#include "request_handler.h"


class HttpProxySession;

class HttpsDirectHandler : public RequestHandler {
public:
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket&> ssl_socket_ref;
    HttpsDirectHandler(HttpProxySession& session, HttpRequestPtr request);
    ~HttpsDirectHandler();

    void HandleRequest();
    void HandleRequest(char *begin, char *end);

private:
    std::string GetSSLPassword();

    void ResolveRemote();
    void OnRemoteConnected(const boost::system::error_code& e);
    void OnRemoteDataSent(const boost::system::error_code& e);
    void OnRemoteStatusLineReceived(const boost::system::error_code& e);
    void OnRemoteHeadersReceived(const boost::system::error_code& e);
    void OnRemoteBodyReceived(const boost::system::error_code& e);
    void OnRemoteChunksReceived(const boost::system::error_code& e);
    void OnLocalDataReceived(const boost::system::error_code& e, std::size_t size);
    void OnLocalDataSent(const boost::system::error_code& e, bool finished);

    bool VerifyCertificate(bool pre_verified, boost::asio::ssl::verify_context& ctx);
    void OnLocalHandshaken(const boost::system::error_code& e);
    void OnRemoteHandshaken(const boost::system::error_code& e);

    HttpProxySession& session_;

    //boost::asio::ip::tcp::socket& local_socket_;
    boost::asio::ssl::context local_ssl_context_;
    ssl_socket_ref local_ssl_socket_; // wrap the local socket
    boost::asio::ssl::context remote_ssl_context_;
    ssl_socket remote_socket_; // TODO close the socket?
    boost::asio::ip::tcp::resolver resolver_;

    std::vector<char> local_buffer_;
    boost::asio::streambuf remote_buffer_;

    HttpRequestPtr request_;
    HttpResponse response_;
};

#endif // HTTPS_DIRECT_HANDLER_H
