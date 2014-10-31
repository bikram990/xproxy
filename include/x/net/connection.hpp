#ifndef CONNECTION_HPP
#define CONNECTION_HPP

namespace x {
namespace memory { class byte_buffer; }
namespace message { class message; }
namespace net {

class connection : public util::counter<connection>,
                   public std::enable_shared_from_this<connection> {
public:
    connection(session& session)
        : connected_(false),
          stopped_(false),
          socket_(session.service()),
          session_(session) {}

    DEFAULT_DTOR(connection);

    virtual void start() = 0;
    virtual void connect(const std::string& host, short port) = 0;
    virtual void handshake(ssl::certificate ca = ssl::certificate(), DH *dh = nullptr) = 0;

    virtual void read();
    virtual void write(const message::message& message);

    socket_wrapper::socket_type& socket() const;

protected:
    bool connected_;
    bool stopped_;
    std::unique_ptr<socket_wrapper> socket_;
    session& session_;

    std::unique_ptr<codec::message_decoder> decoder_;
    std::unique_ptr<codec::message_encoder> encoder_;
    std::unique_ptr<message::message> message_;
    std::unique_ptr<handler::message_handler> handler_;

private:
    enum { FIXED_BUFFER_SIZE = 8192 };

    boost::asio::io_service& service_;

    std::array<char, FIXED_BUFFER_SIZE> buffer_in_;
    std::list<buffer_ptr> buffer_out_;
};

class client_connection : public connection {
public:
    client_connection(session& session) : connection(session) {}

    DEFAULT_DTOR(client_connection);

    virtual void start();
    virtual void connect(const std::string& host, short port);
    virtual void handshake(ssl::certificate ca = ssl::certificate(), DH *dh = nullptr);
};

class server_connection : public connection {
public:
    server_connection(session& session) : connection(session) {}

    DEFAULT_DTOR(server_connection);

    virtual void start();
    virtual void connect(const std::string& host, short port);
    virtual void handshake(ssl::certificate ca = ssl::certificate(), DH *dh = nullptr);

private:
    boost::asio::ip::tcp::resolver resolver_;
};

} // namespace net
} // namespace x

#endif // CONNECTION_HPP
