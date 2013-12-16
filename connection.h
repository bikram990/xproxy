#ifndef CONNECTION_H
#define CONNECTION_H

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include "decoder.h"
#include "filter_chain.h"
#include "filter_context.h"
#include "log.h"
#include "proxy_server.h"
#include "socket.h"


class Connection : public boost::enable_shared_from_this<Connection>,
                   private boost::noncopyable {
public:
    typedef boost::shared_ptr<Connection> Ptr;
    typedef Connection this_type;

    enum {
        kDefaultBufferSize =2048
    };

    enum ConnectionState {
        kAwaiting,   // waiting for connecting
        kReading,    // reading data from socket
        kConnecting, // connecting to remote peer
        kWriting,    // writing data to socket
        kFiltering   // filter decoded object
    };

    static Ptr *Create(boost::asio::io_service& service);

    virtual ~Connection() {
        if(decoder_) delete decoder_;
        if(socket_) delete socket_;

        if(chain_)
            ProxyServer::FilterChainManager().ReleaseFilterChain(chain_);
    }

    boost::asio::ip::tcp::socket& socket() const {
        return socket_->socket();
    }

    void start() {
        chain_ = ProxyServer::FilterChainManager().RequireFilterChain();
        chain_->FilterContext()->SetClientConnection(shared_from_this());
        AsyncRead();
    }

    void stop() {
        ProxyServer::FilterChainManager().ReleaseFilterChain(chain_);
        chain_ = nullptr;
        socket_->close();
        ProxyServer::ClientConnectionManager().stop(shared_from_this());
    }

    void AsyncRead() {
        if(in_.size() > 0)
            callback(boost::system::error_code(), 0);

        socket_->async_read_some(in_.prepare(kDefaultBufferSize),
                                 boost::bind(&this_type::callback,
                                             shared_from_this(),
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
        become(kReading);
    }

    template<typename BufferSequence>
    void AsyncWrite(boost::shared_ptr<BufferSequence> buffers) {
        if(!connected_) {
            AsyncConnect();
            become(kConnecting);
            return;
        }
        socket_->async_write_some(*buffers, boost::bind(&this_type::callback, shared_from_this(), boost::asio::placeholders::error, 0));
        become(kWriting);
    }

    virtual void StoreRemoteAddress(const std::string& host, short port) = 0;

    virtual void AsyncConnect() = 0;

protected:
    explicit Connection(boost::asio::io_service& service)
        : decoder_(nullptr), socket_(Socket::Create(service)), state_(kAwaiting) {}

    void become(ConnectionState state) {
        state_ = state;
    }

    virtual void callback(const boost::system::error_code& e, std::size_t size = 0) {
        switch(state_) {
        case kReading: {
            XDEBUG << "size: " << size << ", size of in buffer: " << in_.size();
            if(size <= 0) {
                if(in_.size() <= 0) {
                    XERROR << "No data, disconnecting the socket.";
                    // TODO disconnect here
                    return;
                }
            } else {
                in_.commit(size);
            }
            XDEBUG << "size: " << size << ", size of in buffer: " << in_.size();

            HttpObject *object = nullptr;
            Decoder::DecodeResult result = decoder_->decode(in_, &object);
            switch(result) {
            case Decoder::kIncomplete:
                XDEBUG << "Incomplete buffer, continue reading.";
                AsyncRead();
                break;
            case Decoder::kFailure:
                XERROR << "Failed to decode object, return.";
                return;
                // TODO add logic here
            case Decoder::kComplete:
            case Decoder::kFinished:
                if(!object)
                    XERROR << "invalid pointer.";
                FilterHttpObject(object);
                break;
            default:
                break;
            }
            break;
        }
        case kConnecting:
            if(e) {
                XERROR << "Failed to connect to remote peer, message: " << e.message();
                // TODO add logic here
                return;
            }
            connected_ = true;
            break;
        case kWriting:
            if(e) {
                XERROR << "Error occurred during writing to remote peer, message: " << e.message();
                // TODO add logic here
                return;
            }
        default:
            break;
        }
    }

    virtual void FilterHttpObject(HttpObject *object) = 0;

protected:
    Decoder *decoder_;
    boost::asio::streambuf in_;
    FilterChain *chain_;

    bool connected_;
    ConnectionState state_;
    Socket *socket_;
};

typedef boost::shared_ptr<Connection> ConnectionPtr;

#endif // CONNECTION_H
