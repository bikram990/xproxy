#ifndef SESSION_HPP
#define SESSION_HPP

namespace x {
namespace net {

class session : public util::counter<session>,
                public std::enable_shared_from_this<session> {
public:
    enum conn_type {
        CLIENT_SIDE, SERVER_SIDE
    };

    session(const server& server)
        : service_(server.get_service()),
          config_(server.get_config()),
          session_manager_(server.get_session_manager()),
          cert_manager_(server.get_certificate_manager()) {}

    DEFAULT_DTOR(session);

    void start() {
        client_connection_.reset(new client_connection(shared_from_this()));
        server_connection_.reset(new server_connection(shared_from_this()));

        client_connection_->start();
    }

    void stop() {
        session_manager_.erase(shared_from_this());
        #warning add more code here
    }

    void on_connection_stop(conn_ptr connection) {
        // 1. remove from manager
        session_manager_.erase(shared_from_this());

        // 2. notify the other side
        if (client_connection_ == connection)
            server_connection_->stop();
        else if (server_connection_ == connection)
            client_connection_->stop();
        else
            assert(0);
    }

    conn_ptr get_connection(conn_type type) const {
        return type == CLIENT_SIDE ? client_connection_ : server_connection_;
    }

    boost::asio::io_service& get_service() const {
        return service_;
    }

    x::conf::config& get_config() const {
        return config_;
    }

    x::ssl::certificate_manager& get_certificate_manager() const {
        return cert_manager_;
    }

private:
    boost::asio::io_service& service_;
    x::conf::config& config_;
    session_manager& session_manager_;
    x::ssl::certificate_manager& cert_manager_;

    conn_ptr client_connection_;
    conn_ptr server_connection_;

    MAKE_NONCOPYABLE(session);
};

typedef std::shared_ptr<session> session_ptr;

} // namespace net
} // namespace x

#endif // SESSION_HPP
