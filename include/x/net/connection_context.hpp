#ifndef CONNECTION_CONTEXT_HPP
#define CONNECTION_CONTEXT_HPP

#include <boost/asio.hpp>
#include "x/common.hpp"

namespace x {
namespace message { class message; namespace http { class http_request; }}
namespace net {

class connection;

class connection_context : public std::enable_shared_from_this<connection_context> {
public:
    #warning add more here!
    enum state {
        READY, CLIENT_SSL_REPLYING, CLIENT_HANDSHAKE, SERVER_CONNECTING, SERVER_WRITING
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
    void parse_destination(const message::http::http_request& request,
                           bool& https, std::string& host, unsigned short& port);

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
