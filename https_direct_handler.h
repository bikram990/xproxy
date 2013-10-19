#ifndef HTTPS_DIRECT_HANDLER_H
#define HTTPS_DIRECT_HANDLER_H

#include "http_client.h"
#include "request_handler.h"


class HttpProxySession;

class HttpsDirectHandler : public RequestHandler {
public:
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket&> ssl_socket_ref;

    HttpsDirectHandler(HttpProxySession& session, HttpRequestPtr request);
    ~HttpsDirectHandler();

    void HandleRequest();

private:
    void OnLocalSSLReplySent(const boost::system::error_code& e);
    void OnLocalHandshaken(const boost::system::error_code& e);
    void OnLocalDataReceived(const boost::system::error_code& e, std::size_t size);
    void OnResponseReceived(const boost::system::error_code& e, HttpResponse *response = NULL);
    void OnLocalDataSent(const boost::system::error_code& e);

    HttpProxySession& session_;
    boost::asio::ssl::context& local_ssl_context_;
    ssl_socket_ref local_ssl_socket_; // wrap the local socket
    boost::asio::ssl::context remote_ssl_context_;

    char local_buffer_[4096]; // TODO do not hard code
    int total_size_;

    HttpRequestPtr request_;
    HttpClient client_;
};

#endif // HTTPS_DIRECT_HANDLER_H
