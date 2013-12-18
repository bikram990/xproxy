#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <boost/atomic.hpp>
#include <boost/lexical_cast.hpp>
#include "connection.h"
#include "decoder.h"
#include "filter_chain.h"
#include "http_container.h"
#include "http_request_decoder.h"

#define LDEBUG XDEBUG << identifier() << " "
#define LERROR XERROR << identifier() << " "
#define LWARN  XWARN  << identifier() << " "

class ClientConnection : public Connection {
public:
    typedef ClientConnection this_type;

    static Connection *create(boost::asio::io_service& service) {
        ++counter_;
        boost::shared_ptr<ClientConnection> connection(new ClientConnection(service));
        connection->connected_ = true;
        connection->FilterContext()->SetConnection(connection);
        return connection.get();
    }

    virtual void start() {
        SetRemoteAddress(socket_->socket().remote_endpoint().address().to_string(),
                         socket_->socket().remote_endpoint().port());
        PostAsyncReadTask();
    }

    virtual void stop() {
        socket_->close();
        ProxyServer::ClientConnectionManager().stop(shared_from_this());
    }

private:

    virtual void AsyncConnect() {
        LWARN << "This function should never be called.";
    }

    virtual void AsyncInitSSLContext() {
        static std::string response("HTTP/1.1 200 Connection Established\r\nConnection: Keep-Alive\r\n\r\n");

        become(kSSLReplying);
        socket_->async_write_some(boost::asio::buffer(response),
                                  boost::bind(&this_type::callback, shared_from_this(), boost::asio::placeholders::error));
    }

private:
    static boost::atomic<std::size_t> counter_;

    explicit ClientConnection(boost::asio::io_service& service)
        : Connection(service), id_(counter_) {
        InitDecoder();
        InitFilterChain();
    }

    virtual void InitDecoder() {
        decoder_ = new HttpRequestDecoder;
    }

    virtual void InitFilterChain() {
        chain_ = new FilterChain(Filter::kRequest);
        chain_->RegisterAll(ProxyServer::FilterChainManager().BuiltinFilters());
    }

    // It is OK to use the parent's HandleReading() function
    // virtual void HandleReading(const boost::system::error_code& e);

    virtual void HandleConnecting(const boost::system::error_code& e) {
        LWARN << "This function should never be called.";
    }

    virtual void HandleWriting(const boost::system::error_code& e) {
        if(e) {
            LERROR << "Error occurred during writing to remote peer, message: " << e.message();
            // TODO add logic here
            return;
        }
        LDEBUG << "Data has been written to socket.";
        // TODO add logic for last writing
    }

    // It is OK to use the parent's HandleSSLReplying() function
    // virtual void HandleSSLReplying(const boost::system::error_code& e);

    virtual std::string identifier() const {
        return "[ClientConnection:" + std::to_string(id_) + "]";
    }

private:
    std::size_t id_;
};

boost::atomic<std::size_t> ClientConnection::counter_(0);

#undef LDEBUG
#undef LERROR
#undef LWARN

#endif // CLIENT_CONNECTION_H
