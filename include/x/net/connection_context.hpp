#ifndef CONNECTION_CONTEXT_HPP
#define CONNECTION_CONTEXT_HPP

#include <boost/asio.hpp>
#include "x/common.hpp"

namespace x {
namespace message { class message; }
namespace net {

class connection;

class connection_context : public std::enable_shared_from_this<connection_context> {
public:
    #warning add more here!
    enum state {
        READY, CLIENT_HANDSHAKE,
    };

    connection_context(boost::asio::io_service& service)
        : https_(false),
          state_(READY),
          service_(service) {}

    boost::asio::io_service& service() const {
        return service_;
    }

    void set_client_connection(std::shared_ptr<connection> connection) {
        client_conn_ = connection;
    }

    void set_server_connection(std::shared_ptr<connection> connection) {
        server_conn_ = connection;
    }

    void on_client_message(message::message& msg);

    void on_server_message(message::message& msg);

private:
    bool https_;
    state state_;

    boost::asio::io_service& service_;
    std::weak_ptr<connection> client_conn_;
    std::weak_ptr<connection> server_conn_;

    MAKE_NONCOPYABLE(connection_context);
};

typedef std::shared_ptr<connection_context> context_ptr;

} // namespace net
} // namespace x

#endif // CONNECTION_CONTEXT_HPP
