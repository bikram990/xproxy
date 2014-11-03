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

    static session_ptr create(boost::asio::io_service& service, session_manager& manager) {
        return session_ptr(new session(service, manager));
    }

    DEFAULT_DTOR(session);

    void start() {
        client_connection_->start();
    }

    void stop() {
        manager_.erase(shared_from_this());
    }

    conn_ptr get_connection(conn_type type) const {
        return type == CLIENT_SIDE ? client_connection_ : server_connection_;
    }

private:
    session(boost::asio::io_service& service, session_manager& manager)
        : service_(service), manager_(manager),
          client_connection_(service),
          server_connection_(service) {}

private:
    boost::asio::io_service& service_;
    session_manager& manager_;

    conn_ptr client_connection_;
    conn_ptr server_connection_;

    MAKE_NONCOPYABLE(session);
};

typedef std::shared_ptr<session> session_ptr;

} // namespace net
} // namespace x

#endif // SESSION_HPP
