#ifndef SOCKET_WRAPPER_HPP
#define SOCKET_WRAPPER_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "x/common.hpp"
#include "x/log/log.hpp"
#include "x/ssl/certificate_manager.hpp"

namespace x {
namespace net {

class socket_wrapper {
public:
    typedef socket_wrapper this_type;
    typedef boost::asio::ip::tcp::socket socket_type;
    typedef boost::asio::ssl::stream<socket_type> ssl_socket_type;
    typedef boost::asio::ssl::stream<socket_type&> ssl_socket_ref_type;
    typedef boost::asio::ssl::context ssl_context_type;

    static this_type *create(boost::asio::io_service& service) {
        return new socket_wrapper(service);
    }

    DEFAULT_DTOR(socket_wrapper);

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
    void switch_to_ssl(boost::asio::ssl::stream_base::handshake_type type, ssl::certificate ca, DH *dh) {
        assert(!use_ssl_);

        ssl_context_.reset(new ssl_context_type(ssl_context_type::sslv23));

        if (type == boost::asio::ssl::stream_base::server) {
            assert(ca.key() && ca.certificate() && dh);

            ssl_context_->set_options(ssl_context_type::default_workarounds
                                      | ssl_context_type::no_sslv2
                                      | ssl_context_type::single_dh_use);
            SSL_CTX_use_certificate(ssl_context_->native_handle(), ca.certificate());
            SSL_CTX_use_PrivateKey(ssl_context_->native_handle(), ca.key());
            SSL_CTX_set_tmp_dh(ssl_context_->native_handle(), dh);
        }

        ssl_socket_.reset(new ssl_socket_ref_type(*socket_, *ssl_context_));

        if (type == boost::asio::ssl::stream_base::client) {
            ssl_socket_->set_verify_mode(boost::asio::ssl::verify_peer);
            ssl_socket_->set_verify_callback([this] (bool preverified, boost::asio::ssl::verify_context& ctx) {
                #warning enhance this function
                char subject_name[256];
                X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
                X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
                XDEBUG << "Verifying server certificate, subject name: " << subject_name
                       << ", pre-verified value: " << preverified;

                return true;
            });
        }

        use_ssl_ = true;
        handshake_type_ = type;
    }

    template<typename HandshakeHandler>
    void async_handshake(HandshakeHandler&& handler) {
        assert(use_ssl_);
        ssl_socket_->async_handshake(handshake_type_, handler);
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
    socket_wrapper(boost::asio::io_service& service)
        : service_(service),
          use_ssl_(false),
          socket_(new socket_type(service)) {}

private:
    boost::asio::io_service& service_;
    bool use_ssl_;
    boost::asio::ssl::stream_base::handshake_type handshake_type_;

    std::unique_ptr<socket_type> socket_;
    std::unique_ptr<ssl_context_type> ssl_context_;
    std::unique_ptr<ssl_socket_ref_type> ssl_socket_;

private:
    MAKE_NONCOPYABLE(SocketFacade);
};

} // namespace net
} // namespace x

#endif // SOCKET_WRAPPER_HPP
