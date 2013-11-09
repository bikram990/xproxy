#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <boost/function.hpp>
#include "http_proxy_session.h"

class HttpClientManager;
class HttpRequest;
class HttpResponse;

class HttpClient : private boost::noncopyable {
    friend class HttpClientManager;
public:
    typedef boost::shared_ptr<HttpClient> Ptr;
    typedef boost::asio::ip::tcp::socket socket_type;
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket_type;
    typedef boost::function<void(const boost::system::error_code&)> callback_type;

    enum State {
        kInUse,
        kAvailable
    };

    ~HttpClient();

    State state() { return state_; }
    void state(State state) { state_ = state; }
    void AsyncSendRequest(HttpProxySession::Mode mode,
                          HttpRequest *request,
                          HttpResponse *response,
                          callback_type callback);

private:
    HttpClient(boost::asio::io_service& service);

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
    bool persistent_;
    std::auto_ptr<socket_type> socket_;
    std::auto_ptr<boost::asio::ssl::context> ssl_context_;
    std::auto_ptr<ssl_socket_type> ssl_socket_;

    boost::asio::streambuf remote_buffer_;

    HttpRequest *request_;
    HttpResponse *response_;

    std::string host_;
    short port_;

    callback_type callback_;
};

#endif // HTTP_CLIENT_H
