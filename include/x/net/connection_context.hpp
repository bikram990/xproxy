#ifndef CONNECTION_CONTEXT_HPP
#define CONNECTION_CONTEXT_HPP

namespace x {
namespace message { class message; }
namespace net {

class connection_context {
public:
    #warning add more here!
    enum state {
        CREATED, READY,
    };

    connection_context(boost::asio::io_service& service)
        : https_(false),
          state_(CREATED),
          service_(service) {}

    boost::asio::io_service& service() const {
        return service_;
    }

    void set_client_connection(conn_ptr connection) {
        client_conn_ = connection;
    }

    void set_server_connection(conn_ptr connection) {
        server_conn_ = connection;
    }

    void on_message(message::message& msg);

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
