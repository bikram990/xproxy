#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/function.hpp>
#include "http_request.h"
#include "http_response.h"


class HttpClient {
public:
    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
    typedef boost::function<void(const boost::system::error_code&, HttpResponse*)> handler_type;

    HttpClient(boost::asio::io_service& service,
               HttpRequest& request,
               boost::asio::ssl::context *context = NULL);
    ~HttpClient();

    void AsyncSendRequest(handler_type handler);

private:
    void ResolveRemote();
    void OnRemoteConnected(const boost::system::error_code& e);
    void OnRemoteHandshaken(const boost::system::error_code& e);
    void OnRemoteDataSent(const boost::system::error_code& e);
    void OnRemoteStatusLineReceived(const boost::system::error_code& e);
    void OnRemoteHeadersReceived(const boost::system::error_code& e);
    void OnRemoteChunksReceived(const boost::system::error_code& e);
    void OnRemoteBodyReceived(const boost::system::error_code& e);

    bool VerifyCertificate(bool pre_verified, boost::asio::ssl::verify_context& ctx);

    boost::asio::io_service& service_;
    boost::asio::ip::tcp::resolver resolver_;

    bool is_ssl_;
    std::auto_ptr<socket> socket_;
    std::auto_ptr<ssl_socket> ssl_socket_;

    boost::asio::streambuf remote_buffer_;

    HttpRequest& request_;
    HttpResponse response_;

    std::string host_;
    short port_;

    handler_type handler_;
};

#endif // HTTP_CLIENT_H
