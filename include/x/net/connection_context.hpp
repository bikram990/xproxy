#ifndef CONNECTION_CONTEXT_HPP
#define CONNECTION_CONTEXT_HPP

namespace x {
namespace net {

class connection_context {
public:
    #warning add more here!
    enum state {
        READY,
    };

    void on_message(message::message& msg);

private:
    bool https_;
    state state_;

    std::string client_host_;
    unsigned short client_port_;
    std::string server_host_;
    unsigned short server_port_;

    boost::asio::io_service& service_;
    std::weak_ptr<connection> client_conn_;
    std::weak_ptr<connection> server_conn_;

    MAKE_NONCOPYABLE(connection_context);
};

typedef std::shared_ptr<connection_context> context_ptr;

} // namespace net
} // namespace x

#endif // CONNECTION_CONTEXT_HPP
