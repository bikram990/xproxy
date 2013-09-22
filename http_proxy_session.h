#ifndef HTTP_PROXY_SESSION_H
#define HTTP_PROXY_SESSION_H

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "http_direct_handler.h"
#include "http_proxy_request.h"
#include "http_proxy_response.h"
#include "request_handler.h"


class HttpProxySessionManager;

class HttpProxySession
        : public boost::enable_shared_from_this<HttpProxySession>,
          private boost::noncopyable {
public:
    HttpProxySession(boost::asio::io_service& service,
                     HttpProxySessionManager& manager);
    ~HttpProxySession();
    boost::asio::ip::tcp::socket& LocalSocket() { return local_socket_; }
    void Start();
    void Stop();
    void Terminate();

private:
    void HandleLocalRead(const boost::system::error_code& e, std::size_t size);

    boost::asio::io_service& service_;
    boost::asio::ip::tcp::socket local_socket_;
    boost::asio::ip::tcp::socket remote_socket_;
    HttpProxySessionManager& manager_;

    boost::array<char, 4096> local_buffer_;

    // TODO make the handler a generic handler after all types implemented
    boost::shared_ptr<RequestHandler> handler_;
};

typedef boost::shared_ptr<HttpProxySession> HttpProxySessionPtr;

#endif // HTTP_PROXY_SESSION_H
