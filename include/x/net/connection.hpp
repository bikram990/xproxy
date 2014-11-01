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
          session_(session),
          writing_(false) {}
          #warning add more constructions here

    DEFAULT_DTOR(connection);

    virtual void start() = 0;
    virtual void connect() = 0;
    virtual void handshake(ssl::certificate ca = ssl::certificate(), DH *dh = nullptr) = 0;

    virtual void read() {
        XDEBUG_WITH_ID(this) << "=> read()";

        assert(connected_);
        assert(!stopped_);

        auto self(shared_from_this());
        boost::asio::async_read(*socket,
                                boost::asio::buffer(buffer_in_),
                                boost::asio::transfer_at_lease(1),
                                [self, this] (const boost::system::error_code& e, std::size_t length) {
            if (e) {
                XERROR_WITH_ID(this) << "read error, code: " << e.value()
                                        << ", message: " << e.message();
                stop();
                return;
            }

            auto consumed = decoder_->decode(buffer_in_, length, *message_);
            assert(consumed == length);

            if (message_->deliverable()) {
                handler_->handle_message(*message_);
                return;
            }

            read();
        });

        XDEBUG_WITH_ID(this) << "<= read()";
    }

    virtual void write(const message::message& message) {
        XDEBUG_WITH_ID(this) << "=> write()";

        buffer_ptr buf(new memory::byte_buffer);
        encoder_->encode_message(message, buf);

        buffer_out_.push_back(buf);
        do_write();

        XDEBUG_WITH_ID(this) << "<= write()";
    }

    socket_wrapper::socket_type& socket() const {
        return socket_->socket();
    }

    void set_host(const std::string& host) {
        host_ = host;
    }

    void set_port(short port) {
        port_ = port;
    }

protected:
    bool connected_;
    bool stopped_;
    std::string host_;
    short port_;
    std::unique_ptr<socket_wrapper> socket_;
    session& session_;

    std::unique_ptr<codec::message_decoder> decoder_;
    std::unique_ptr<codec::message_encoder> encoder_;
    std::unique_ptr<message::message> message_;
    std::unique_ptr<handler::message_handler> handler_;

private:
    void do_write() {
        if (writing_) return;
        if (buffer_out_.empty()) return;

        XDEBUG_WITH_ID(this) << "=> do_write()";

        writing_ = true;
        auto& candidate = buffer_out_.front();
        auto self(shared_from_this());
        socket_->async_write_some(boost::asio::buffer(candidate->data(), candidate->size(),
                                                      [self, candidate, this] (const boost::system::error_code& e, std::size_t length) {
            if (e) {
                XERROR_WITH_ID(this) << "write error, code: " << e.value()
                                     << ", message: " << e.message();
                stop();
                return;
            }

            writing_ = false;
            if (length < candidate->size()) {
                XERROR_WITH_ID(this) << "write incomplete, continue.";
                candidate->erase(0, length);
                do_write();
                return;
            }

            buffer_out_.erase(buffer_out_.begin());
            if (!buffer_out_.empty()) {
                XDEBUG_WITH_ID(this) << "more buffers added, continue.";
                do_write();
                return;
            }

            #warning add something here
        }));

        XDEBUG_WITH_ID(this) << "<= do_write()";
    }

    enum { FIXED_BUFFER_SIZE = 8192 };

    boost::asio::io_service& service_;

    std::array<char, FIXED_BUFFER_SIZE> buffer_in_;
    std::list<buffer_ptr> buffer_out_;
    bool writing_;
};

class client_connection : public connection {
public:
    client_connection(session& session) : connection(session) {}

    DEFAULT_DTOR(client_connection);

    virtual void start() {
        connected_ = true;
        host_ = socket_->socket().remote_endpoint().address().to_string();
        port_ = socket_->socket().remote_endpoint().port();
        read();
    }

    virtual void connect() {
        assert(0);
    }

    virtual void handshake(ssl::certificate ca = ssl::certificate(), DH *dh = nullptr) {
        XDEBUG_WITH_ID(this) << "=> handshake()";

        auto self(shared_from_this());
        socket_->switch_to_ssl(boost::asio::ssl::stream_base::client, ca, dh);
        socket_->async_handshake([self, this] (const boost::system::error_code& e) {
            #warning do something here
        });

        XDEBUG_WITH_ID(this) << "<= handshake()";
    }
};

class server_connection : public connection {
public:
    server_connection(session& session) : connection(session) {}

    DEFAULT_DTOR(server_connection);

    virtual void start() {
        assert(host_.length() > 0);
        assert(port_ != 0);

        connect();
    }

    virtual void connect() {
        XDEBUG_WITH_ID(this) << "=> connect()";

        using namespace boost::asio::ip;
        auto self(shared_from_this());
        resolver_.async_resolve(tcp::resolver::query(host_, port_),
                                [self, this] (const boost::system::error_code& e, tcp::resolver::iterator it) {
            if (e) {
                XERROR_WITH_ID(this) << "resolve error, code: " << e.value()
                                     << ", message: " << e.message();
                stop();
                return;
            }

            socket_->async_connect(it, [self, this] (const boost::system::error_code& e, tcp::resolver::iterator it) {
                if (e) {
                    XERROR_WITH_ID(this) << "connect error, code: " << e.value()
                                         << ", message: " << e.message();
                    stop();
                    return;
                }

                connected_ = true;
                #warning add something here
            });
        });

        XDEBUG_WITH_ID(this) << "<= connect()";
    }

    virtual void handshake(ssl::certificate ca = ssl::certificate(), DH *dh = nullptr) {
        XDEBUG_WITH_ID(this) << "=> handshake()";

        auto self(shared_from_this());
        socket_->switch_to_ssl(boost::asio::ssl::stream_base::server, ca, dh);
        socket_->async_handshake([self, this] (const boost::system::error_code& e) {
            #warning do something here
        });

        XDEBUG_WITH_ID(this) << "<= handshake()";
    }

private:
    boost::asio::ip::tcp::resolver resolver_;
};

} // namespace net
} // namespace x

#endif // CONNECTION_HPP
