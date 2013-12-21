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

    enum {
        kDefaultTimeout = 60 // 60 second
    };

    static ConnectionPtr create(boost::asio::io_service& service) {
        ++counter_;
        boost::shared_ptr<ClientConnection> connection(new ClientConnection(service));
        connection->connected_ = true;
        connection->FilterContext()->SetConnection(connection);
        return connection;
    }

    virtual ~ClientConnection() {
        /// we cannot call the virtual function identifier() here, so we should
        /// compose the debug string ourselves
        XDEBUG << "[ClientConnection:" << id_ << "] destructing...";
    }

    virtual void start() {
        SetRemoteAddress(socket_->socket().remote_endpoint().address().to_string(),
                         socket_->socket().remote_endpoint().port());
        PostAsyncReadTask();
    }

    virtual void stop() {
        connected_ = false;
        socket_->close();

        /// because we still have a connection shared_ptr and a bridged
        /// connection shared_ptr in filter context, so do not forget to reset
        /// it here to free the connection resource
        chain_->FilterContext()->reset(true);

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
                                  boost::bind(&this_type::HandleSSLReplying, shared_from_this(), boost::asio::placeholders::error));
    }

    virtual void Cleanup(bool disconnect) {
        if(disconnect) {
            stop();
            return;
        }

        reset();

        timer_.expires_from_now(boost::posix_time::seconds(kDefaultTimeout));
        timer_.async_wait(boost::bind(&this_type::HandleTimeout,
                                      shared_from_this(),
                                      boost::asio::placeholders::error));

        /// after the resetting, we start a new reading task
        /// but this task will timeout after 60 seconds if client does not send
        /// a request
        PostAsyncReadTask();
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

    virtual void HandleReading(const boost::system::error_code& e) {
        if(in_.size() <= 0) {
            // TODO disconnect here or do nothing??
            LDEBUG << "No data in socket, does the browser want to keep the connection alive?";
            // PostAsyncReadTask();
            return;
        }

        /// cancel the timer
        timer_.cancel();

        LDEBUG << "Content in raw buffer[size: " << in_.size() << "]:\n"
               << boost::asio::buffer_cast<const char *>(in_.data());

        HttpObject *object = nullptr;
        Decoder::DecodeResult result = decoder_->decode(in_, &object);

        switch(result) {
        case Decoder::kIncomplete:
            LDEBUG << "Incomplete buffer, continue reading...";
            PostAsyncReadTask();
            break;
        case Decoder::kComplete:
            LDEBUG << "Decoded one object, continue decoding...";
            chain_->FilterContext()->container()->AppendObject(object);
            PostAsyncReadTask();
            break;
        case Decoder::kFailure:
            LERROR << "Failed to decode object, return.";
            // TODO add logic here
            break;
        case Decoder::kFinished:
            if(!object) {
                LERROR << "Invalid object pointer, but this should never happen.";
                // TODO add logic here
                return;
            }
            become(kFiltering); // TODO do we really need this state?
            chain_->FilterContext()->container()->AppendObject(object);
            chain_->filter();
            LDEBUG << "The filtering process finished.";
            break;
        default:
            LERROR << "Invalid result: " << static_cast<int>(result);
            break;
        }
    }

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
