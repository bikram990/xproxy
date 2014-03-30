#ifndef SESSION_H
#define SESSION_H

#include <boost/asio.hpp>
#include "counter.h"

class ClientConnection;
class HttpMessage;
class SessionManager;
class ProxyServer;
class ServerConnection;

class Session : public Counter<Session>, public std::enable_shared_from_this<Session> {
public:
    typedef Session this_type;

    Session(ProxyServer& server);
    virtual ~Session();

    boost::asio::io_service& service() const { return service_; }

    bool stopped() const { return stopped_; }

    std::shared_ptr<class ClientConnection> ClientConnection() { return client_connection_; }

    void init();

    void start();
    void destroy();
    void stop();

    void OnRequestComplete(std::shared_ptr<HttpMessage> request);

    /*
     * if response is very large, e.g. a video, we need to send the data that
     * received to browser and meanwhile, receiving more data, we should not
     * cache the response until totally completed, that may need to browser
     * a very long time waiting
     * so, there are two response related callback
     */
    void OnResponse(std::shared_ptr<HttpMessage> response);
    void OnResponseComplete(std::shared_ptr<HttpMessage> response);

private:
    void InitClientSSLContext();

private:
    ProxyServer& server_;
    SessionManager& manager_;

    boost::asio::io_service& service_;

    bool stopped_;

    std::shared_ptr<class ClientConnection> client_connection_;
    std::shared_ptr<ServerConnection> server_connection_;
};

#endif // SESSION_H
