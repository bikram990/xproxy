#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include "connection.h"
#include "decoder.h"
#include "filter_chain.h"
#include "http_container.h"
#include "http_request_decoder.h"

class ClientConnection : public Connection {
public:
    explicit ClientConnection(boost::asio::io_service& service)
        : Connection(service) {
        InitDecoder();
        InitFilterChain();
    }

    virtual void start() {
        AsyncRead();
    }

    virtual void stop() {
        socket_->close();
        ProxyServer::ClientConnectionManager().stop(shared_from_this());
    }

    virtual void AsyncConnect() {}

protected:
    virtual void InitDecoder() {
        decoder_ = new HttpRequestDecoder;
    }

    virtual void InitFilterChain() {
        // TODO add logic here
        chain_ = new FilterChain(Filter::kRequest);
        chain_->FilterContext()->SetConnection(shared_from_this());
        chain_->RegisterAll(ProxyServer::FilterChainManager().BuiltinFilters());
        // chain_->RegisterFilter();
    }

    virtual void FilterHttpObject(HttpObject *object) {
        if(!chain_) {
            XERROR << "The filter chain is not set.";
            return;
        }
        if(!object) {
            XERROR << "Invalid HttpObject pointer.";
            return;
        }

        chain_->FilterContext()->container()->AppendObject(object);
        chain_->filter();

        become(kFiltering);
    }

    // void Callback(const boost::system::error_code& e, std::size_t size = 0) {
    //     switch(state_) {
    //     case kReading: {
    //         XDEBUG << "Read data from socket, size: " << size;
    //         in_.commit(size);

    //         HttpObject *object = nullptr;
    //         Decoder::DecodeResult result = decoder_->decode(in_, &object);

    //         switch(result) {
    //         case Decoder::kIncomplete:
    //             AsyncRead();
    //             break;
    //         case Decoder::kFailure:
    //             // TODO add logic here
    //         case Decoder::kComplete:
    //         case Decoder::kFinished:
    //             // TODO add logic here
    //             chain_->FilterContext()->RequestContainer()->AppendObject(object);
    //             chain_->FilterRequest();
    //             break;
    //         default:
    //             break;
    //         }
    //         break;
    //     }
    //     // TODO add later
    //     default:
    //         ; // add colon to pass the compilation
    //     }
    // }
};

#endif // CLIENT_CONNECTION_H
