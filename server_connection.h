#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include <boost/atomic.hpp>
#include <boost/lexical_cast.hpp>
#include "connection.h"
#include "decoder.h"
#include "filter_chain.h"
#include "http_container.h"
#include "http_response_decoder.h"

#define LDEBUG XDEBUG << identifier() << " "
#define LERROR XERROR << identifier() << " "
#define LWARN  XWARN  << identifier() << " "

class ServerConnection : public Connection {
public:
    typedef ServerConnection this_type;

    static Connection *create(boost::asio::io_service& service) {
        ++counter_;
        return new ServerConnection(service);
    }

    virtual void start() {
        chain_->FilterContext()->SetConnection(shared_from_this());
    }

    virtual void stop() {}

    virtual void AsyncConnect() {
        try {
            become(kConnecting);
            boost::asio::ip::tcp::resolver::query query(host_, boost::lexical_cast<std::string>(port_));
            boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query);

            LDEBUG << "Connecting to remote address: " << endpoint_iterator->endpoint().address();

            socket_->async_connect(endpoint_iterator, boost::bind(&this_type::callback,
                                                                  shared_from_this(),
                                                                  boost::asio::placeholders::error));
        } catch(const boost::system::system_error& e) {
            LERROR << "Failed to resolve [" << host_ << ":" << port_ << "], error: " << e.what();
            // TODO add logic here
        }
    }

private:
    static boost::atomic<std::size_t> counter_;

    explicit ServerConnection(boost::asio::io_service& service)
        : Connection(service), id_(counter_), resolver_(service) {
        InitDecoder();
        InitFilterChain();
    }

    virtual void InitDecoder() {
        decoder_ = new HttpResponseDecoder;
    }

    virtual void InitFilterChain() {
        chain_ = new FilterChain(Filter::kResponse);
        chain_->RegisterAll(ProxyServer::FilterChainManager().BuiltinFilters());
    }

    // It is OK to use the parent's HandleReading() function
    // virtual void HandleReading(const boost::system::error_code& e);

    // It is OK to use the parent's HandleConnecting() function
    // virtual void HandleConnecting(const boost::system::error_code& e);

    // It is OK to use the parent's HandleConnecting() function
    // virtual void HandleWriting(const boost::system::error_code& e);

    virtual std::string identifier() const {
        return "[ServerConnection:" + std::to_string(id_) + "]";
    }

private:
    std::size_t id_;
    boost::asio::ip::tcp::resolver resolver_;
};

boost::atomic<std::size_t> ServerConnection::counter_(0);

#undef LDEBUG
#undef LERROR
#undef LWARN

#endif // SERVER_CONNECTION_H
