#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <boost/shared_ptr.hpp>
#include "http_request.h"

class HttpProxySession;


class RequestHandler {
public:
    typedef HttpRequest::State ResultType;
    typedef boost::shared_ptr<HttpRequest> HttpRequestPtr;

    static RequestHandler *CreateHandler(char *data, std::size_t size,
                                         HttpProxySession& session,
                                         boost::asio::io_service& service,
                                         boost::asio::ip::tcp::socket& local_socket,
                                         boost::asio::ip::tcp::socket& remote_socket);
    virtual void HandleRequest() = 0;
    virtual ~RequestHandler();

protected:
    RequestHandler();

private:
    static RequestHandler *CreateHttpHandler(char *data, std::size_t size,
                                             HttpProxySession& session,
                                             boost::asio::io_service& service,
                                             boost::asio::ip::tcp::socket& local_socket,
                                             boost::asio::ip::tcp::socket& remote_socket);
};

#endif // REQUEST_HANDLER_H
