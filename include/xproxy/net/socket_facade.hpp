#ifndef SOCKET_FACADE_HPP
#define SOCKET_FACADE_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "xproxy/common.hpp"
#include "xproxy/log/log.hpp"

namespace xproxy {
namespace net {

/**
 * @brief The SocketFacade class
 *
 * This class encapsulates both ssl socket and normal socket.
 * The functions defined in this class with different naming
 * style, is to make this class work with boost asio function
 * series async_xxx(...).
 */
class SocketFacade {
public:
    typedef SocketFacade this_type;
    typedef boost::asio::ip::tcp::socket socket_type;
    typedef boost::asio::ssl::stream<socket_type> ssl_socket_type;
    typedef boost::asio::ssl::stream<socket_type&> ssl_socket_ref_type;
    typedef boost::asio::ssl::context ssl_context_type;

    enum SSLMode { kServer, kClient };

    static this_type *create(boost::asio::io_service& service) {
        return new SocketFacade(service);
    }

    DEFAULT_VIRTUAL_DTOR(SocketFacade);

public:
    const socket_type& socket() const {
        if (use_ssl_)
            assert(&ssl_socket_->next_layer() == socket_.get());
        return *socket_;
    }

    socket_type& socket() {
        if (use_ssl_)
            assert(&ssl_socket_->next_layer() == socket_.get());
        return *socket_;
    }

    const socket_type::lowest_layer_type& lowest_layer() const {
        return socket_->lowest_layer();
    }

    socket_type::lowest_layer_type& lowest_layer() {
        return socket_->lowest_layer();
    }

    bool is_open() const {
        return lowest_layer().is_open();
    }

    void close() const {
        if (is_open()) {
            try {
                socket_->shutdown(socket_type::shutdown_both);
                socket_->close();
            } catch(boost::system::system_error& e) {
                XERROR << "socket close: " << e.what();
            } catch(...) {} //ignore exceptions
        }
    }

    template<typename SettableSocketOption>
    void set_option(const SettableSocketOption& option) {
        socket_->set_option(option);
    }

public:
    template<typename SharedCAPtr, typename SharedDHPtr, typename HandshakeHandler>
    void useSSL(HandshakeHandler&& handler, SSLMode mode = kClient,
                SharedCAPtr ca = nullptr, SharedDHPtr dh = nullptr) {
        if (use_ssl_) {
            // if the ssl socket is already set up, we generate an error here
            // TODO could we use move in capture here?
            socket_->get_io_service().post([handler]() {
                handler(boost::asio::error::already_open);
            });
            return;
        }

        ssl_context_.reset(new ssl_context_type(ssl_context_type::sslv23));

        if (mode == kServer) {
            if (!ca || !dh) {
                // TODO add logic here
                use_ssl_ = false;
                return;
            }
            ssl_context_->set_options(ssl_context_type::default_workarounds
                                      | ssl_context_type::no_sslv2
                                      | ssl_context_type::single_dh_use);
            ::SSL_CTX_use_certificate(ssl_context_->native_handle(), ca->cert);
            ::SSL_CTX_use_PrivateKey(ssl_context_->native_handle(), ca->key);
            ::SSL_CTX_set_tmp_dh(ssl_context_->native_handle(), dh->dh);
        }

        ssl_socket_.reset(new ssl_socket_ref_type(*socket_, *ssl_context_));

        if (mode == kClient) {
            ssl_socket_->set_verify_mode(boost::asio::ssl::verify_peer);
            ssl_socket_->set_verify_callback([this] (bool preverified, boost::asio::ssl::verify_context& ctx) {
                    // TODO enhance this function
                    char subject_name[256];
                    X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
                    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
                    XDEBUG << "Verifying server certificate, subject name: " << subject_name
                           << ", pre-verified value: " << preverified;

                    return true;
            });
        }

        use_ssl_ = true;

        ssl_socket_->async_handshake(mode == kClient ? boost::asio::ssl::stream_base::client :
                                                       boost::asio::ssl::stream_base::server,
                                     [this, handler](const boost::system::error_code& e) {
            // if error occurred, we disable ssl read/write here
            if (e) {
                use_ssl_ = false;
                XERROR << "Handshake error: " + e.message();
            }
            handler(e);
        });
    }

    template<typename Iterator, typename ConnectHandler>
    void async_connect(Iterator begin, ConnectHandler&& handler) {
        boost::asio::async_connect(lowest_layer(), begin, handler);
    }

    template<typename MutableBufferSequence, typename ReadHandler>
    void async_read_some(const MutableBufferSequence& buffers, ReadHandler&& handler) {
        if (use_ssl_)
            ssl_socket_->async_read_some(buffers, handler);
        else
            socket_->async_read_some(buffers, handler);
    }

    template<typename ConstBufferSequence, typename WriteHandler>
    void async_write(ConstBufferSequence& buffers, WriteHandler&& handler) {
        if (use_ssl_)
            boost::asio::async_write(*ssl_socket_, buffers, handler);
        else
            boost::asio::async_write(*socket_, buffers, handler);
    }

    template<typename ConstBufferSequence, typename WriteHandler>
    void async_write_some(const ConstBufferSequence& buffers, WriteHandler&& handler) {
        if (use_ssl_)
            ssl_socket_->async_write_some(buffers, handler);
        else
            socket_->async_write_some(buffers, handler);
    }

private:
    SocketFacade(boost::asio::io_service& service)
        : service_(service),
          use_ssl_(false),
          socket_(new socket_type(service)) {}

private:
    boost::asio::io_service& service_;
    bool use_ssl_;

    std::unique_ptr<socket_type> socket_;
    std::unique_ptr<ssl_context_type> ssl_context_;
    std::unique_ptr<ssl_socket_ref_type> ssl_socket_;

private:
    MAKE_NONCOPYABLE(SocketFacade);
};

} // namespace net
} // namespace xproxy

#endif // SOCKET_FACADE_HPP
