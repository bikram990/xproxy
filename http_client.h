#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "http_request.h"
#include "http_response.h"
#include "request_handler.h"


class HttpClient {
public:
    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

    HttpClient(boost::asio::io_service& service,
               HttpRequest::Ptr request,
               HttpResponse::Ptr response,
               boost::asio::ssl::context *context = NULL);
    ~HttpClient();

    HttpClient& host(const std::string& host) { host_ = host; return *this; }
    HttpClient& port(short port) { port_ = port; return *this; }
    HttpClient& request(HttpRequest::Ptr request) { request_ = request; return *this; }
    void AsyncSendRequest(RequestHandler::handler_type handler);

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

    HttpRequest::Ptr request_;
    HttpResponse::Ptr response_;

    std::string host_;
    short port_;

    RequestHandler::handler_type handler_;
};

#endif // HTTP_CLIENT_H
