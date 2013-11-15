#ifndef HTTP_PROXY_SESSION_H
#define HTTP_PROXY_SESSION_H

#include <boost/asio/ssl.hpp>
#include <boost/atomic.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "http_request.h"
#include "http_response.h"
#include "socket.h"

class HttpProxySession
    : public boost::enable_shared_from_this<HttpProxySession>,
      private boost::noncopyable {
public:
    typedef boost::shared_ptr<HttpProxySession> Ptr;
    typedef HttpProxySession this_type;

    enum State {
        kWaiting,
        kContinueTransferring,
        kCompleted,
        kSSLReplying, // only stands for reply of CONNECT method
        kFetching,
        kSSLWaiting,
        kReplying // stands for all http/https replies
    };

    enum Mode {
        HTTP,
        HTTPS
    };

    static HttpProxySession *Create(boost::asio::io_service& service) {
        ++counter_;
        return new HttpProxySession(service);
    }

    ~HttpProxySession();

    State state() const { return state_; }
    Mode mode() const { return mode_; }
    boost::asio::ip::tcp::socket& LocalSocket() { return local_socket_->socket(); }
    HttpRequest *Request() { return request_.get(); }
    HttpResponse *Response() { return response_.get(); }

    void Start();
    void Stop();
    void Terminate();

    void OnResponseReceived(const boost::system::error_code& e); // the callback

private:
    HttpProxySession(boost::asio::io_service& service);

    void OnRequestReceived(const boost::system::error_code& e, std::size_t size);
    void OnSSLReplySent(const boost::system::error_code& e);
    void OnResponseSent(const boost::system::error_code& e);

    void ContinueReceiving();
    void InitSSLContext();
    void SendResponse(HttpResponse& response);
    void reset();

    static boost::atomic<std::size_t> counter_;

    std::size_t id_;

    State state_;
    Mode mode_;
    bool persistent_;

    boost::asio::io_service::strand strand_;
    std::unique_ptr<Socket> local_socket_;

    boost::asio::streambuf local_buffer_;

    std::auto_ptr<HttpRequest> request_;
    std::auto_ptr<HttpResponse> response_;
};

#endif // HTTP_PROXY_SESSION_H
