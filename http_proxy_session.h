#ifndef HTTP_PROXY_SESSION_H
#define HTTP_PROXY_SESSION_H

#include <boost/asio/ssl.hpp>
#include "http_response.h"
#include "request_handler.h"


class HttpProxySessionManager;

class HttpProxySession
        : public boost::enable_shared_from_this<HttpProxySession>,
          private boost::noncopyable {
public:
    typedef boost::shared_ptr<HttpProxySession> Ptr;
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket&> ssl_socket_ref;
    typedef HttpProxySession this_type;

    enum State {
        kWaiting,
        kContinueTransferring,
        kCompleted,
        kSSLReplying, // only stands for reply of CONNECT method
        kFetching,
        kSSLHandshaking,
        kSSLWaiting,
        kReplying // stands for all http/https replies
    };

    enum Mode {
        HTTP,
        HTTPS
    };

    HttpProxySession(boost::asio::io_service& service,
                     HttpProxySessionManager& manager);
    ~HttpProxySession();
    State state() const { return state_; }
    Mode mode() const { return mode_; }
    boost::asio::io_service& service() { return service_; }
    boost::asio::ip::tcp::socket& LocalSocket() { return local_socket_; }
    void Start();
    void Stop();
    void Terminate();

private:
    void OnRequestReceived(const boost::system::error_code& e, std::size_t size);
    void OnSSLReplySent(const boost::system::error_code& e);
    void OnHandshaken(const boost::system::error_code& e);
    void OnResponseReceived(const boost::system::error_code& e);
    void OnResponseSent(const boost::system::error_code& e);

    void ContinueReceiving();
    void InitSSLContext();
    void SendResponse(HttpResponse& response);
    void reset();

    State state_;
    Mode mode_;
    bool persistent_;

    boost::asio::io_service& service_;
    boost::asio::io_service::strand strand_;
    boost::asio::ip::tcp::socket local_socket_;
    std::auto_ptr<boost::asio::ssl::context> local_ssl_context_;
    std::auto_ptr<ssl_socket_ref> local_ssl_socket_;
    HttpProxySessionManager& manager_;

    boost::asio::streambuf local_buffer_;

    HttpRequest::Ptr request_;
    HttpResponse::Ptr response_;
    RequestHandler::Ptr handler_;
};

#endif // HTTP_PROXY_SESSION_H
