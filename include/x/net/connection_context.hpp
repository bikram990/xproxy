#ifndef CONNECTION_CONTEXT_HPP
#define CONNECTION_CONTEXT_HPP

#include <boost/asio.hpp>
#include "x/common.hpp"

namespace x {
namespace message { class message; namespace http { class http_request; }}
namespace net {

enum connection_event {
    CONNECT, READ, HANDSHAKE, WRITE
};

class server;
class connection;
class client_connection;
class server_connection;

class connection_context : public std::enable_shared_from_this<connection_context> {
public:
    connection_context(server& svr)
        : https_(false),
          ssl_setup_(false),
          message_exchange_completed_(false),
          server_(svr) {}

    boost::asio::io_service& service() const;

    void reset();

    void set_client_connection(std::shared_ptr<connection> connection) {
        client_conn_ = connection;
    }

    void set_server_connection(std::shared_ptr<connection> connection) {
        server_conn_ = connection;
    }

    void on_event(connection_event event, client_connection& conn);
    void on_event(connection_event event, server_connection& conn);

private:
    void on_client_message(message::message& msg);
    void on_server_message(message::message& msg);

    void parse_destination(const message::http::http_request& request,
                           bool& https, std::string& host, unsigned short& port);

    bool https_;
    bool ssl_setup_;
    bool message_exchange_completed_;

    server& server_;
    std::weak_ptr<connection> client_conn_;
    std::weak_ptr<connection> server_conn_;

    MAKE_NONCOPYABLE(connection_context);
};

typedef std::shared_ptr<connection_context> context_ptr;

} // namespace net
} // namespace x

#endif // CONNECTION_CONTEXT_HPP
