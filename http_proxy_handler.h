#ifndef HTTP_PROXY_HANDLER_H
#define HTTP_PROXY_HANDLER_H

#include "http_client.h"
#include "request_handler.h"


class HttpProxySession;

class HttpProxyHandler : public RequestHandler {
public:
    HttpProxyHandler(HttpProxySession& session, HttpRequestPtr request);
    ~HttpProxyHandler() { TRACE_THIS_PTR; }

    void HandleRequest();

private:
    void BuildProxyRequest(HttpRequest& request);
    void OnResponseReceived(const boost::system::error_code& e, HttpResponse *response = NULL);
    void OnLocalDataSent(const boost::system::error_code& e);

    HttpProxySession& session_;
    boost::asio::ip::tcp::socket& local_socket_;
    boost::asio::ssl::context remote_ssl_context_;

    HttpRequestPtr origin_request_;
    HttpRequest proxy_request_;

    HttpClient client_;
};

#endif // HTTP_PROXY_HANDLER_H
