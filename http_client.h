#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <boost/atomic.hpp>
#include <boost/function.hpp>
#include "http_proxy_session.h"
#include "socket.h"

class HttpClientManager;
class HttpRequest;
class HttpResponse;

class HttpClient : private boost::noncopyable {
public:
    typedef boost::shared_ptr<HttpClient> Ptr;
    typedef boost::function<void(const boost::system::error_code&)> callback_type;

    enum State {
        kInUse,
        kAvailable
    };

    static HttpClient *Create(boost::asio::io_service& service) {
        ++counter_;
        return new HttpClient(service);
    }

    ~HttpClient();

    std::size_t id() { return id_; }
    State state() { return state_; }
    void state(State state) { state_ = state; }
    void AsyncSendRequest(HttpProxySession::Mode mode,
                          HttpRequest *request,
                          HttpResponse *response,
                          callback_type callback);

private:
    HttpClient(boost::asio::io_service& service);

    void InvokeCallback(const boost::system::error_code& e) {
        callback_(e);

        // TODO should we close socket when any error occurs, or
        // just when eof or short read occurs?
        if(e == boost::asio::error::eof || SSL_SHORT_READ(e)) {
            socket_->close();
            persistent_ = false;
        }

        state_ = kAvailable;
    }

    void ResolveRemote();
    void OnRemoteConnected(const boost::system::error_code& e);
    void OnRemoteDataSent(const boost::system::error_code& e);
    void OnRemoteStatusLineReceived(const boost::system::error_code& e);
    void OnRemoteHeadersReceived(const boost::system::error_code& e);
    void OnRemoteChunksReceived(const boost::system::error_code& e);
    void OnRemoteBodyReceived(const boost::system::error_code& e);

    static boost::atomic<std::size_t> counter_;

    std::size_t id_; // the http client id, it will do good for debugging, and for statistic analysis

    State state_;

    boost::asio::io_service& service_;
    boost::asio::io_service::strand strand_;
    boost::asio::ip::tcp::resolver resolver_;

    bool is_ssl_;
    bool persistent_;
    std::unique_ptr<Socket> socket_;

    boost::asio::streambuf remote_buffer_;

    HttpRequest *request_;
    HttpResponse *response_;

    std::string host_;
    short port_;

    callback_type callback_;
};

#endif // HTTP_CLIENT_H
