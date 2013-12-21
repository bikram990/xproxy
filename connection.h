#ifndef CONNECTION_H
#define CONNECTION_H

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include "decoder.h"
#include "filter_chain.h"
#include "filter_context.h"
#include "log.h"
#include "proxy_server.h"
#include "resource_manager.h"
#include "socket.h"

#define LDEBUG XDEBUG << identifier() << " "
#define LERROR XERROR << identifier() << " "
#define LWARN  XWARN  << identifier() << " "

class Connection : public Resettable,
                   public boost::enable_shared_from_this<Connection>,
                   private boost::noncopyable {
public:
    typedef Connection this_type;

    enum {
        kDefaultBufferSize = 8192
    };

    enum ConnectionState {
        kAwaiting,   // waiting for connecting
        kReading,    // reading data from socket
        kConnecting, // connecting to remote peer
        kWriting,    // writing data to socket
        kSSLReplying,// writing SSL reply to browser
        kFiltering   // filter decoded object
    };

    virtual ~Connection() {
        if(decoder_) delete decoder_;
        if(socket_) delete socket_;
        if(chain_) delete chain_;
    }

    /**
     * @brief reset
     *
     * Reset the server connection
     *
     * If a connection need to be reused, it should be reset first.
     */
    virtual void reset() {
        decoder_->reset();

        /// reset the context, but do not forget to set the connection back
        chain_->FilterContext()->reset();
        /// we do not need this now, because we have not reset the connection
        /// in reset() of FilterContext
        // chain_->FilterContext()->SetConnection(shared_from_this());

        state_ = kAwaiting;

        if(in_.size() > 0)
            in_.consume(in_.size());
    }

public: // getters

    boost::asio::io_service& service() const { return service_; }

    boost::asio::ip::tcp::socket& socket() const {
        return socket_->socket();
    }

    const std::string& host() const { return host_; }

    unsigned short port() const { return port_; }

    class FilterContext *FilterContext() const { return chain_->FilterContext(); }

    /// Return a string which can identify current connection.
    /**
     * This function is mainly used for debugging, the ideal invocation result
     * of this function should be something like: "[ServerConnection:1]"
     *
     * @brief identifier
     * @return a string which can identify current connection
     */
    virtual std::string identifier() const = 0;

public: // setters

    void SetRemoteAddress(const std::string& host, unsigned short port) {
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

    void PostSSLInitTask() {
        service_.post(boost::bind(&this_type::AsyncInitSSLContext, shared_from_this()));
    }

    void PostCleanupTask(bool persistent_connection = true) {
        service_.post(boost::bind(&this_type::Cleanup, shared_from_this(), !persistent_connection));
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

        /// by default, asio only set the buffer size to 512, it is very very
        /// inefficient, we manually set it to a larger value here
        in_.prepare(kDefaultBufferSize);
        boost::asio::async_read(*socket_, in_, boost::asio::transfer_at_least(1),
                                boost::bind(&this_type::callback, shared_from_this(), boost::asio::placeholders::error));
    }

    void AsyncWriteStart(boost::shared_ptr<std::vector<SharedBuffer>> buffers) {
        /// cancel the timer_ before writing for persistent server connection
        /// for client connection, we do not need to cancel timer_ here, but
        /// it does not affect
        timer_.cancel();

        // clear the out buffer first
        if(out_.size() > 0)
            out_.consume(out_.size());

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

    /// For ServerConnection & ClientConnection, The process is not the same, so
    /// we make this function pure virtual
    virtual void AsyncInitSSLContext() = 0;

    /**
     * Handle cleanup work
     *
     * For persistent connection, disconnect is false, and the timer_ will start
     * until timed out, for nonpersistent connection, we close the socket.
     *
     * @brief Cleanup
     * @param disconnect whether to disconnect the socket
     */
    virtual void Cleanup(bool disconnect) = 0;

protected: // instantiation

    explicit Connection(boost::asio::io_service& service)
        : service_(service), socket_(Socket::Create(service)),
          decoder_(nullptr), chain_(nullptr),
          connected_(false), state_(kAwaiting), timer_(service) {}

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
        case kSSLReplying:
            HandleSSLReplying(e);
            break;
        default:
            LERROR << "The connection is in a wrong state: "
                   << static_cast<int>(state_);
            break;
        }
    }

    virtual void HandleTimeout(const boost::system::error_code& e) {
        if(e == boost::asio::error::operation_aborted)
            LDEBUG << "The timeout timer is cancelled.";
        else if(e)
            LERROR << "Error occurred with the timer, message: " << e.message();
        else if(timer_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
            LDEBUG << "Connection timed out, close it.";
            stop();
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
            // TODO disconnect here or do nothing??
            LDEBUG << "No data in socket, does the browser want to keep the connection alive?";
            // PostAsyncReadTask();
            return;
        }

        LDEBUG << "Content in raw buffer[size: " << in_.size() << "]:\n"
               << boost::asio::buffer_cast<const char *>(in_.data());

        HttpObject *object = nullptr;
        Decoder::DecodeResult result = decoder_->decode(in_, &object);

        switch(result) {
        case Decoder::kIncomplete:
            LDEBUG << "Incomplete buffer, continue reading...";
            PostAsyncReadTask();
            break;
        case Decoder::kFailure:
            LERROR << "Failed to decode object, return.";
            // TODO add logic here
            break;
        case Decoder::kComplete:
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

    virtual void HandleSSLReplying(const boost::system::error_code& e) {
        if(e) {
            LERROR << "Error occurred during writing SSL OK reply to socket, message: " << e.message();
            // TODO add logic here
            return;
        }

        InitSSLContext();

        // because we have decoded "CONNECT..." request, so we reset it here
        // to decode new request
        decoder_->reset();

        // also reset the container, to clear previously decoded http object
        chain_->FilterContext()->container()->reset();

        become(kReading);
        AsyncRead();
    }

    virtual void InitSSLContext() {
        if(!chain_->FilterContext()->BridgedConnection()) {
            LERROR << "The bridged connection is not set, but this should never happen.";
            return;
        }

        auto ca = ResourceManager::GetCertManager().GetCertificate(chain_->FilterContext()->BridgedConnection()->host());
        auto dh = ResourceManager::GetCertManager().GetDHParameters();
        socket_->SwitchProtocol(kHttps, kServer, ca, dh);
    }

protected:
    boost::asio::io_service& service_;
    Socket *socket_;
    Decoder *decoder_;
    FilterChain *chain_;
    bool connected_;
    ConnectionState state_;
    std::string host_;
    unsigned short port_;
    boost::asio::streambuf in_;
    boost::asio::streambuf out_;

    /**
     * Persistent connection timeout checker
     *
     * For client connection, we consider all connection as persistent
     * connection; for server connection, we decide according to response
     * header "Connection".
     *
     * @brief timer_
     */
    boost::asio::deadline_timer timer_;
};

typedef boost::shared_ptr<Connection> ConnectionPtr;

#undef LDEBUG
#undef LERROR
#undef LWARN

#endif // CONNECTION_H
