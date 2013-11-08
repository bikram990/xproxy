#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/function.hpp>

class HttpRequest;
class HttpResponse;

class HttpClient {
public:
    typedef boost::asio::ip::tcp::socket socket_type;
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket_type;
    typedef boost::function<void(const boost::system::error_code&)> callback_type;

    enum State {
        kInUse,
        kAvailable
    };

    HttpClient(boost::asio::io_service& service,
               boost::asio::ssl::context *context = NULL);
    ~HttpClient();

    State state() { return state_; }
    void AsyncSendRequest(HttpRequest *request, HttpResponse *response, callback_type callback);

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

    State state_;

    boost::asio::io_service& service_;
    boost::asio::io_service::strand strand_;
    boost::asio::ip::tcp::resolver resolver_;

    bool is_ssl_;
    std::auto_ptr<socket_type> socket_;
    std::auto_ptr<ssl_socket_type> ssl_socket_;

    boost::asio::streambuf remote_buffer_;

    HttpRequest *request_;
    HttpResponse *response_;

    std::string host_;
    short port_;

    callback_type callback_;
};

#endif // HTTP_CLIENT_H
