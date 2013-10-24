#ifndef HTTP_PROXY_SESSION_H
#define HTTP_PROXY_SESSION_H

#include <boost/asio.hpp>
#include "request_handler.h"


class HttpProxySessionManager;

class HttpProxySession
        : public boost::enable_shared_from_this<HttpProxySession>,
          private boost::noncopyable {
public:
    HttpProxySession(boost::asio::io_service& service,
                     HttpProxySessionManager& manager);
    ~HttpProxySession();
    boost::asio::io_service& service() { return service_; }
    boost::asio::ip::tcp::socket& LocalSocket() { return local_socket_; }
    void Start();
    void Stop();
    void Terminate();

private:
    void OnRequestReceived(const boost::system::error_code& e, std::size_t size);

    boost::asio::io_service& service_;
    boost::asio::ip::tcp::socket local_socket_;
    HttpProxySessionManager& manager_;

    boost::array<char, 4096> local_buffer_;

    // TODO make the handler a generic handler after all types implemented
    boost::shared_ptr<RequestHandler> handler_;
};

typedef boost::shared_ptr<HttpProxySession> HttpProxySessionPtr;

#endif // HTTP_PROXY_SESSION_H
