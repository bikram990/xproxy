#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include "connection.h"
#include "decoder.h"
#include "filter_chain.h"
#include "http_response_decoder.h"

class ServerConnection : public Connection {
public:
    typedef ServerConnection this_type;

    explicit ServerConnection(boost::asio::io_service& service)
        : Connection(service) {
        decoder_ = new HttpResponseDecoder();
    }

    virtual void StoreRemoteAddress(const std::string& host, short port) {
        host_ = host;
        port_ = port;
    }

    virtual void AsyncConnect() {
        try {
            boost::asio::ip::tcp::resolver::query query(host_, boost::lexical_cast<std::string>(port_));
            boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query);

            XDEBUG << "Connecting to remote address: " << endpoint_iterator->endpoint().address();

            socket_->async_connect(endpoint_iterator, boost::bind(&this_type::Callback,
                                                                               this,
                                                                               boost::asio::placeholders::error));
            become(kConnecting);
        } catch(const boost::system::system_error& e) {
            XERROR << "Failed to resolve [" << host_ << ":" << port_ << "], error: " << e.what();
            // TODO add logic here
        }
    }

    virtual void Callback(const boost::system::error_code& e, std::size_t size = 0) {
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
        case kWriting:
            if(e) {
                XERROR << "Error writing.";
                // TODO add logic here
                return;
            }
            AsyncRead();
        case kConnecting:
            if(e) {
                XERROR << "Failed to connect to remote server, message: " << e.message();
                // TODO add logic here
                return;
            }
            connected_ = true;
            AsyncWrite(); // TODO correct invocation here
            break;
        // TODO add later
        default:
        }
    }

private:
    std::string host_;
    short port_;
};

#endif // SERVER_CONNECTION_H
