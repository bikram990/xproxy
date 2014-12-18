#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio.hpp>
#include "x/common.hpp"
#include "x/net/connection.hpp"
#include "x/net/connection_manager.hpp"

namespace x {
namespace conf { class config; }
namespace ssl { class certificate_manager; }
namespace net {

class server {
public:
    const static unsigned short DEFAULT_SERVER_PORT = 7077;

    server();

    DEFAULT_DTOR(server);

    bool init();

    void start();

    boost::asio::io_service& get_service() {
        return service_;
    }

    x::conf::config& get_config() {
        return *config_;
    }

    x::ssl::certificate_manager& get_certificate_manager() const {
        return *cert_manager_;
    }

    x::net::connection_manager& get_client_connection_manager() const {
        return *client_conn_mgr_;
    }

    x::net::connection_manager& get_server_connection_manager() const {
        return *server_conn_mgr_;
    }

private:
    void init_signal_handler();

    void init_acceptor();

    void start_accept();

    unsigned short port_;

    boost::asio::io_service service_;
    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;

    std::unique_ptr<x::conf::config> config_;
    std::unique_ptr<x::ssl::certificate_manager> cert_manager_;
    std::unique_ptr<x::net::connection_manager> client_conn_mgr_;
    std::unique_ptr<x::net::connection_manager> server_conn_mgr_;

    connection_ptr current_connection_;

    MAKE_NONCOPYABLE(server);
};

} // namespace net
} // namespace x

#endif // SERVER_HPP
