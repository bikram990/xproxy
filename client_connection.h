#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include "connection.h"
#include "decoder.h"
#include "filter_chain.h"
#include "http_request_decoder.h"

class ClientConnection : public Connection {
protected:
    explicit ClientConnection(boost::asio::io_service& service)
        : Connection(service) {
        decoder_ = new HttpRequestDecoder();
    }

    virtual void StoreRemoteAddress(const std::strign& host, short port) {}

    void Callback(const boost::system::error_code& e, std::size_t size = 0) {
        switch(state_) {
        case kReading:
            in_.commit(size);

            HttpObject *object = nullptr;
            Decoder::DecodeResult result = decoder_->decode(in_, &object);

            switch(result) {
            case Decoder::kIncomplete:
                AsyncRead();
                break;
            case Decoder::kFailure:
                // TODO add logic here
            case Decoder::kComplete:
            case Decoder::kFinished:
                // TODO add logic here
                chain_->RequestContainer()->AppendObject(object);
                chain_->filter();
                break;
            default:
                break;
            }
            break;
        // TODO add later
        default:
        }
    }
};

#endif // CLIENT_CONNECTION_H
