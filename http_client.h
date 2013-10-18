#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "http_request.h"
#include "http_response.h"


class HttpClient {
public:
    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

    HttpClient(boost::asio::io_service& service,
               HttpRequest& request,
               boost::asio::ssl::context *context = NULL);
    ~HttpClient();

    template<typename HandlerT>
    void AsyncSendRequest(const HandlerT& handler);

private:
    void ResolveRemote();
    void OnRemoteConnected(const boost::system::error_code& e);
    void OnRemoteHandshaken(const boost::system::error_code& e);
    void OnRemoteDataSent(const boost::system::error_code& e);
    void OnRemoteStatusLineReceived(const boost::system::error_code& e);
    void OnRemoteHeadersReceived(const boost::system::error_code& e);
    void OnRemoteChunksReceived(const boost::system::error_code& e);
    void OnRemoteBodyReceived(const boost::system::error_code& e);

    boost::asio::io_service& service_;
    boost::asio::ip::tcp::resolver resolver_;

    bool is_ssl_;
    std::auto_ptr<socket> socket_;
    std::auto_ptr<ssl_socket> ssl_socket_;

    boost::asio::streambuf remote_buffer_;

    HttpRequest& request_;
    HttpResponse response_;
};

#endif // HTTP_CLIENT_H
