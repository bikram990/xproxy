#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <list>
#include <boost/asio.hpp>
#include "x/net/connection_context.hpp"
#include "x/net/socket_wrapper.hpp"
#include "x/ssl/certificate_manager.hpp"
#include "x/util/counter.hpp"

namespace x {
namespace codec { class message_decoder; class message_encoder; }
namespace handler { class message_handler; }
namespace memory { class byte_buffer; }
namespace message { class message; }
namespace net {

class connection : public util::counter<connection>,
                   public std::enable_shared_from_this<connection> {
public:
    #warning we should use connection state instead of connected_/stopped_ later
    enum conn_state {
        BEGINNING, CONNECTED, READING, HANDSHAKING,
        DECODING, HANDLING, ENCODING, WRITING, COMPLETED,
        DISCONNECTED, STOPPED
    };

    connection(boost::asio::io_service& service);

    DEFAULT_DTOR(connection);

    virtual void start() = 0;
    virtual void connect() = 0;
    virtual void handshake(ssl::certificate ca = ssl::certificate(), DH *dh = nullptr) = 0;

    virtual void read();

    virtual void write(const message::message& message);

    void stop();

    socket_wrapper::socket_type& socket() const {
        return socket_->socket();
    }

    context_ptr get_context() const {
        return context_;
    }

    void set_host(const std::string& host) {
        host_ = host;
    }

    void set_port(unsigned short port) {
        port_ = port;
    }

protected:
    bool connected_;
    bool stopped_;
    std::string host_;
    unsigned short port_;
    std::unique_ptr<socket_wrapper> socket_;
    context_ptr context_;

    std::unique_ptr<codec::message_decoder> decoder_;
    std::unique_ptr<codec::message_encoder> encoder_;
    std::unique_ptr<message::message> message_;
    std::unique_ptr<handler::message_handler> handler_;

private:
    void do_write();

    enum { FIXED_BUFFER_SIZE = 8192 };

    std::array<char, FIXED_BUFFER_SIZE> buffer_in_;
    std::list<memory::buffer_ptr> buffer_out_;
    bool writing_;
};

class client_connection : public connection {
public:
    client_connection(boost::asio::io_service& service);

    DEFAULT_DTOR(client_connection);

    virtual void start();

    virtual void connect();

    virtual void handshake(ssl::certificate ca = ssl::certificate(), DH *dh = nullptr);
};

class server_connection : public connection {
public:
    server_connection(boost::asio::io_service& service);

    DEFAULT_DTOR(server_connection);

    virtual void start();

    virtual void connect();

    virtual void handshake(ssl::certificate ca = ssl::certificate(), DH *dh = nullptr);

private:
    boost::asio::ip::tcp::resolver resolver_;
};

} // namespace net
} // namespace x

#endif // CONNECTION_HPP
