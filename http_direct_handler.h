#ifndef HTTP_DIRECT_HANDLER_H
#define HTTP_DIRECT_HANDLER_H

#include <boost/enable_shared_from_this.hpp>
#include "http_client.h"
#include "request_handler.h"


class HttpProxySession;

class HttpDirectHandler
    : public RequestHandler,
      public boost::enable_shared_from_this<HttpDirectHandler> {
public:
    HttpDirectHandler(HttpProxySession& session, HttpRequestPtr request);
    ~HttpDirectHandler() { TRACE_THIS_PTR; }

    void HandleRequest();

private:
    void OnResponseReceived(const boost::system::error_code& e, HttpResponse *response = NULL);
    void OnLocalDataSent(const boost::system::error_code& e);

    HttpProxySession& session_;
    boost::asio::ip::tcp::socket& local_socket_;

    HttpRequestPtr request_;
    HttpClient client_;
};

#endif // HTTP_DIRECT_HANDLER_H
