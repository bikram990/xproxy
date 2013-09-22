#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

class HttpProxySession;


class RequestHandler {
public:
    static RequestHandler *CreateHandler(char *data, std::size_t size,
                                         HttpProxySession& session,
                                         boost::asio::io_service& service,
                                         boost::asio::ip::tcp::socket& local_socket,
                                         boost::asio::ip::tcp::socket& remote_socket);
    virtual void HandleRequest(char *data, std::size_t size) = 0;
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
