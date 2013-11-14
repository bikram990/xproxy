#ifndef SOCKET_H
#define SOCKET_H

#include "common.h"
#include "log.h"

class Socket {
public:
    typedef Socket this_type;

    static Socket *Create(boost::asio::io_service& service) {
        return new Socket(service);
    }

    template<typename SharedCAPtr, typename SharedDHPtr>
    void SwitchProtocol(Protocol protocol = kHttps, SSLMode mode = kClient,
                        SharedCAPtr ca = SharedCAPtr(), SharedDHPtr dh = SharedDHPtr()) {
        if(protocol == kHttp) {
            use_ssl_ = false;
            return;
        }

        ssl_context_.reset(new boost::asio::ssl::context(boost::asio::ssl::context::sslv23));
        if(mode == kServer) {
            if(!ca || !dh) {
                // TODO add logic here
                use_ssl_ = false;
                return;
            }
            ssl_context_->set_options(boost::asio::ssl::context::default_workarounds
                                      | boost::asio::ssl::context::no_sslv2
                                      | boost::asio::ssl::context::single_dh_use);
            ::SSL_CTX_use_certificate(ssl_context_->native_handle(), ca->cert);
            ::SSL_CTX_use_PrivateKey(ssl_context_->native_handle(), ca->key);
            ::SSL_CTX_set_tmp_dh(ssl_context_->native_handle(), dh->dh);
        }

        ssl_socket_.reset(new ssl_socket_ref_type(*socket_, *ssl_context_));

        if(mode == kClient) {
            ssl_socket_->set_verify_mode(boost::asio::ssl::verify_peer);
            ssl_socket_->set_verify_callback(boost::bind(&this_type::VerifyCertificate, this, _1, _2));
        }

        use_ssl_ = true;
        ssl_ready_ = false;
        ssl_mode_ = mode;
    }

    socket_type& socket() const {
		if (use_ssl_)
			return ssl_socket_->next_layer();
		else
			return *socket_;
    }

    const socket_type::lowest_layer_type& lowest_layer() const {
        return use_ssl_ ? ssl_socket_->lowest_layer() :
                          socket_->lowest_layer();
    }

    socket_type::lowest_layer_type& lowest_layer() {
        return use_ssl_ ? ssl_socket_->lowest_layer() :
                          socket_->lowest_layer();
    }

    bool is_open() const {
        return use_ssl_? ssl_socket_->lowest_layer().is_open() :
                         socket_->lowest_layer().is_open();
    }

    void close() const {
        if(is_open()) {
            use_ssl_ ? ssl_socket_->shutdown() :
                       socket_->shutdown(socket_type::shutdown_both);
            // ssl socket does not have close() method, use socket's close() instead
            socket().close();
        }
    }

    template<typename Iterator, typename ConnectHandler>
    void async_connect(Iterator begin, ConnectHandler& handler) {
        boost::asio::async_connect(lowest_layer(), begin, handler);
    }

    template<typename MutableBufferSequence, typename ReadHandler>
    void async_read_some(const MutableBufferSequence& buffers, ReadHandler& handler) {
        if(use_ssl_) {
            boost::system::error_code ec;
            ssl_ready_ = PrepareSSLSocket(ec);
            if(!ssl_ready_) {
                // TODO add logic here
                handler(ec, 0);
                return;
            }

            ssl_socket_->async_read_some(buffers, handler);
        } else {
            socket_->async_read_some(buffers, handler);
        }
    }

    template<typename ConstBufferSequence, typename WriteHandler>
    void async_write(ConstBufferSequence& buffers, WriteHandler& handler) {
        if(use_ssl_) {
            boost::system::error_code ec;
            ssl_ready_ = PrepareSSLSocket(ec);
            if(!ssl_ready_) {
                // TODO add logic here
                handler(ec);
                return;
            }

            boost::asio::async_write(*ssl_socket_, buffers, handler);
        } else {
            boost::asio::async_write(*socket_, buffers, handler);
        }
    }

    template<typename ConstBufferSequence, typename WriteHandler>
    void async_write_some(const ConstBufferSequence& buffers, WriteHandler& handler) {
        if(use_ssl_) {
            boost::system::error_code ec;
            ssl_ready_ = PrepareSSLSocket(ec);
            if(!ssl_ready_) {
                // TODO add logic here
                handler(ec, 0);
                return;
            }

            ssl_socket_->async_write_some(buffers, handler);
        } else {
            socket_->async_write_some(buffers, handler);
        }
    }

    std::string to_string() const {
        return "["
                + lowest_layer().local_endpoint().address().to_string()
                + ":"
                + std::to_string(lowest_layer().local_endpoint().port())
                + " <=> "
                + lowest_layer().remote_endpoint().address().to_string()
                + ":"
                + std::to_string(lowest_layer().remote_endpoint().port())
                + "]";
    }

    ~Socket() { close(); }

private:
    Socket(boost::asio::io_service& service)
        : service_(service),
          use_ssl_(false), ssl_ready_(false), ssl_mode_(kClient),
          socket_(new socket_type(service)) {}

    template<typename BufferSequence, typename Handler, typename Callback>
    void handle_handshake(const boost::system::error_code& e,
                          const BufferSequence& buffers,
                          const Handler& handler,
                          const Callback& callback) {
        if(e) {
            // TODO do something here
            return;
        }

        ssl_ready_ = true;
        callback(buffers, handler);
    }

    bool VerifyCertificate(bool pre_verified, boost::asio::ssl::verify_context& ctx) {
        // TODO enhance this function
        char subject_name[256];
        X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
        XDEBUG << "Verify remote certificate, subject name: " << subject_name
               << ", pre_verified value: " << pre_verified;

        return true;
    }

    bool PrepareSSLSocket(boost::system::error_code& e) {
        if(ssl_ready_)
            return true;

        ssl_socket_->handshake(ssl_mode_ == kClient ? boost::asio::ssl::stream_base::client :
                                                      boost::asio::ssl::stream_base::server,
                               e);
        return !e;
    }

    boost::asio::io_service& service_;

    bool use_ssl_;
    bool ssl_ready_; // whether ssl handshake is done and ready for data trasfer
    SSLMode ssl_mode_;

    std::unique_ptr<socket_type> socket_;
    std::unique_ptr<ssl_context_type> ssl_context_;
    std::unique_ptr<ssl_socket_ref_type> ssl_socket_;
};

#endif // SOCKET_H
