#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <boost/asio.hpp>
#include "x/codec/message_decoder.hpp"
#include "x/codec/message_encoder.hpp"
#include "x/memory/byte_buffer.hpp"
#include "x/message/message.hpp"
#include "x/net/connection_context.hpp"
#include "x/net/socket_wrapper.hpp"
#include "x/util/counter.hpp"

namespace x {
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

    connection(context_ptr ctx);

    DEFAULT_DTOR(connection);

    virtual void start() = 0;
    virtual void connect() = 0;
    virtual void handshake(ssl::certificate ca = ssl::certificate(), DH *dh = nullptr) = 0;

    virtual void read();
    virtual void write(const message::message& message);

    virtual void on_connect() = 0;
    virtual void on_read() = 0;
    virtual void on_write() = 0;
    virtual void on_handshake() = 0;


    void stop();

    socket_wrapper::socket_type& socket() const {
        return socket_->socket();
    }

    context_ptr get_context() const {
        return context_;
    }


    message::message& get_message() {
        return *message_;
    }

    const message::message& get_message() const {
        return *message_;
    }

    void set_host(const std::string& host) {
        host_ = host;
    }

    std::string get_host() const {
        return host_;
    }

    void set_port(unsigned short port) {
        port_ = port;
    }

    unsigned short get_port() const {
        return port_;
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

private:
    void do_write();

    enum { FIXED_BUFFER_SIZE = 8192 };

    std::array<char, FIXED_BUFFER_SIZE> buffer_in_;
    std::list<memory::buffer_ptr> buffer_out_;
    bool writing_;
};

class client_connection : public connection {
public:
    client_connection(context_ptr ctx);

    DEFAULT_DTOR(client_connection);

    virtual void start();

    virtual void connect();

    virtual void handshake(ssl::certificate ca = ssl::certificate(), DH *dh = nullptr);

    virtual void on_connect();

    virtual void on_read();

    virtual void on_write();

    virtual void on_handshake();
};

class server_connection : public connection {
public:
    server_connection(context_ptr ctx);

    DEFAULT_DTOR(server_connection);

    virtual void start();

    virtual void connect();

    virtual void handshake(ssl::certificate ca = ssl::certificate(), DH *dh = nullptr);

    virtual void on_connect();

    virtual void on_read();

    virtual void on_write();

    virtual void on_handshake();

private:
    boost::asio::ip::tcp::resolver resolver_;
};

typedef std::shared_ptr<connection> connection_ptr;

} // namespace net
} // namespace x

#endif // CONNECTION_HPP
