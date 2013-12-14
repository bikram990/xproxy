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
        socket_->async_read_some(in_.prepare(kDefaultBufferSize),
                                 boost::bind(&this_type::Callback,
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
        socket_->async_write_some(buffers, boost::bind(&this_type::Callback, shared_from_this(), boost::asio::placeholders::error, 0));
        become(kWriting);
    }

    virtual void StoreRemoteAddress(const std::string& host, short port) = 0;

    virtual void AsyncConnect() = 0;

protected:
    explicit Connection(boost::asio::io_service& service)
        : decoder_(nullptr), in_(kDefaultBufferSize),
          socket_(Socket::Create(service)), state_(kAwaiting) {}

    void become(ConnectionState state) {
        state_ = state;
    }

    virtual void Callback(const boost::system::error_code& e, std::size_t size = 0) = 0;

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
