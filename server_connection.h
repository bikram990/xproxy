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

    enum {
        kDefaultTimeout = 15 // 15 seconds
    };

    static ConnectionPtr create(boost::asio::io_service& service) {
        ++counter_;
        boost::shared_ptr<ServerConnection> connection(new ServerConnection(service));
        connection->FilterContext()->SetConnection(connection);
        return connection;
    }

    virtual void start() {
        LDEBUG << "This function has no use.";
    }

    virtual void stop() {
        connected_ = false;
        socket_->close();

        /// because we still have a connection shared_ptr and a bridged
        /// connection shared_ptr in filter context, so do not forget to reset
        /// it here to free the connection resource
        chain_->FilterContext()->reset();

        /// as no matter we stop the connection or keep it alive, we always need
        /// to return the connection back to connection manager, so we do the
        /// following in Cleanup() function
        // ProxyServer::ServerConnectionManager().ReleaseConnection(shared_from_this());
    }

private:

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

    virtual void AsyncInitSSLContext() {
        socket_->SwitchProtocol<ResourceManager::CertManager::CAPtr,
                ResourceManager::CertManager::DHParametersPtr>(kHttps);
    }

    virtual void Cleanup(bool disconnect) {
        /// no matter disconnect or not, we should return the connection to
        /// connection manager first
        ProxyServer::ServerConnectionManager().ReleaseConnection(shared_from_this());

        if(disconnect) {
            stop();
            return;
        }

        /// if this is a persistent connection, we just "reset" the connection

        decoder_->reset();

        // reset the context here, but do not forget to set the connection
        // pointer back
        chain_->FilterContext()->reset();
        chain_->FilterContext()->SetConnection(shared_from_this());

        state_ = kAwaiting;

        if(in_.size() > 0)
            in_.consume(in_.size());

        timer_.expires_from_now(boost::posix_time::seconds(kDefaultTimeout));
        timer_.async_wait(boost::bind(&this_type::HandleTimeout,
                                      shared_from_this(),
                                      boost::asio::placeholders::error));
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

    virtual void HandleSSLReplying(const boost::system::error_code& e) {
        LERROR << "This function should never be called.";
    }

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
