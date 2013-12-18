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

#define LDEBUG XDEBUG << identifier() << " "
#define LERROR XERROR << identifier() << " "
#define LWARN  XWARN  << identifier() << " "

class Connection : public boost::enable_shared_from_this<Connection>,
                   private boost::noncopyable {
public:
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

    virtual ~Connection() {
        if(decoder_) delete decoder_;
        if(socket_) delete socket_;
        if(chain_) delete chain_;
    }

public: // getters

    boost::asio::io_service& service() const { return service_; }

    boost::asio::ip::tcp::socket& socket() const {
        return socket_->socket();
    }

    const std::string& host() const { return host_; }

    short port() const { return port_; }

    class FilterContext *FilterContext() const { return chain_->FilterContext(); }

public: // setters

    void SetRemoteAddress(const std::string& host, short port) {
        host_ = host;
        port_ = port;
    }

public: // async tasks

    void PostAsyncReadTask() {
        become(kReading);
        service_.post(boost::bind(&this_type::AsyncRead, shared_from_this()));
    }

    void PostAsyncWriteTask(boost::shared_ptr<std::vector<SharedBuffer>> buffers) {
        /// We do not set state here, because currently the connection may be
        /// not connected, so let the state be decided later
        service_.post(boost::bind(&this_type::AsyncWriteStart, shared_from_this(), buffers));
    }

public:

    virtual void start() = 0;

    virtual void stop() = 0;

protected: // real async IO tasks

    void AsyncRead() {
        /// Although the state is set in PostAsyncReadTask(), we set it again
        /// here to ensure it is correctly set
        become(kReading);

        if(in_.size() > 0) {
            LDEBUG << "There is still data in buffer, skip reading from socket.";
            callback(boost::system::error_code());
            return;
        }

        boost::asio::async_read(*socket_, in_, boost::asio::transfer_at_least(1),
                                boost::bind(&this_type::callback, shared_from_this(), boost::asio::placeholders::error));
    }

    void AsyncWriteStart(boost::shared_ptr<std::vector<SharedBuffer>> buffers) {
        std::for_each(buffers->begin(), buffers->end(),
                      [this](SharedBuffer buffer) {
                          std::size_t copied = boost::asio::buffer_copy(out_.prepare(buffer->size()), boost::asio::buffer(buffer->data(), buffer->size()));
                          if(copied != buffer->size()) {
                              LERROR << "Data length mismatch, but this should never happen.";
                              return;
                          }
                          out_.commit(copied);
                      });

        if(!connected_) {
            LDEBUG << "The connection is not connected, connecting...";
            become(kConnecting);
            AsyncConnect();
            return;
        }

        become(kWriting);
        AsyncWrite();
    }

    void AsyncWrite() {
        become(kWriting);
        socket_->async_write_some(boost::asio::buffer(out_.data(), out_.size()),
                                  boost::bind(&this_type::callback, shared_from_this(), boost::asio::placeholders::error));
    }

    virtual void AsyncConnect() = 0;

protected: // instantiation

    explicit Connection(boost::asio::io_service& service)
        : service_(service), socket_(Socket::Create(service)),
          decoder_(nullptr), chain_(nullptr),
          connected_(false), state_(kAwaiting) {}

    virtual void InitDecoder() = 0;

    virtual void InitFilterChain() = 0;

protected:

    void become(ConnectionState state) {
        state_ = state;
    }

    virtual void callback(const boost::system::error_code& e) {
        LDEBUG << "Function callback() called, current connection state: "
               << static_cast<int>(state_);
        switch(state_) {
        case kReading:
            HandleReading(e);
            break;
        case kConnecting:
            HandleConnecting(e);
            break;
        case kWriting:
            HandleWriting(e);
            break;
        default:
            LERROR << "The connection is in a wrong state: "
                   << static_cast<int>(state_);
            break;
        }
    }

    /// The handler for kReading state in callback() function
    /**
     * @brief HandleReading
     *
     * Derived classes should override this function, if its logic cannot
     * meet the requirement.
     *
     * Same as below functions: HandleConnecting(), HandleWriting().
     */
    virtual void HandleReading(const boost::system::error_code& e) {
        if(in_.size() <= 0) {
            LERROR << "No data, disconnecting the socket.";
            // TODO disconnect here
            return;
        }

        LDEBUG << "Read data from socket, buffer size: " << in_.size();

        LDEBUG << "Content in raw buffer:\n"
               << boost::asio::buffer_cast<const char *>(in_.data());

        HttpObject *object = nullptr;
        Decoder::DecodeResult result = decoder_->decode(in_, &object);

        switch(result) {
        case Decoder::kIncomplete:
            LDEBUG << "Incomplete buffer, continue reading.";
            PostAsyncReadTask();
            break;
        case Decoder::kFailure:
            LERROR << "Failed to decode object, return.";
            // TODO add logic here
            break;
        case Decoder::kComplete:
        case Decoder::kFinished:
            if(!object) {
                LERROR << "The decoding is finished, but the pointer is invalid.";
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
        if(e) {
            LERROR << "Error occurred during connecting to remote peer, message: " << e.message();
            // TODO add logic here
            return;
        }

        LDEBUG << "Remote peer connected: " << host_ << ":" << port_;
        connected_ = true;
        AsyncWrite();
    }

    virtual void HandleWriting(const boost::system::error_code& e) {
        if(e) {
            LERROR << "Error occurred during writing to remote peer, message: " << e.message();
            // TODO add logic here
            return;
        }
        LDEBUG << "Data has been written to socket, now start reading...";
        PostAsyncReadTask();
    }

    /// Return a string which can identify current connection.
    /**
     * This function is mainly used for debugging, the ideal invocation result
     * of this function should be something like: "[ServerConnection:1]"
     *
     * @brief identifier
     * @return a string which can identify current connection
     */
    virtual std::string identifier() const = 0;

protected:
    boost::asio::io_service& service_;
    Socket *socket_;
    Decoder *decoder_;
    FilterChain *chain_;
    bool connected_;
    ConnectionState state_;
    std::string host_;
    short port_;
    boost::asio::streambuf in_;
    boost::asio::streambuf out_;
};

typedef boost::shared_ptr<Connection> ConnectionPtr;

#undef LDEBUG
#undef LERROR
#undef LWARN

#endif // CONNECTION_H
