#ifndef SESSION_HPP
#define SESSION_HPP

#include <boost/asio.hpp>
#include "x/util/counter.hpp"

namespace x {
namespace conf { class config; }
namespace ssl { class certificate_manager; }
namespace net {

class server;
class session_manager;

class session : public util::counter<session>,
                public std::enable_shared_from_this<session> {
public:
    enum conn_type {
        CLIENT_SIDE, SERVER_SIDE
    };

    session(server& server);

    DEFAULT_DTOR(session);

    void start();

    void stop();

    void on_connection_stop(conn_ptr connection);

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

} // namespace net
} // namespace x

#endif // SESSION_HPP
