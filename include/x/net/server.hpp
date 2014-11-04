#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio.hpp>
#include "x/common.hpp"

namespace x {
namespace conf { class config; }
namespace ssl { class certificate_manager; }
namespace net {

class session;
class session_manager;

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

    session_manager& get_session_manager() {
        return *session_manager_;
    }

    x::ssl::certificate_manager& get_certificate_manager() const {
        return *cert_manager_;
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
    std::unique_ptr<session_manager> session_manager_;
    std::unique_ptr<x::ssl::certificate_manager> cert_manager_;

    std::shared_ptr<session> current_session_;

    MAKE_NONCOPYABLE(server);
};

} // namespace net
} // namespace x

#endif // SERVER_HPP
