#ifndef SERVER_HPP
#define SERVER_HPP

namespace x {
namespace net {

class server {
public:
    server(unsigned short port = 7077)
        : port_(port),
          signals_(service_),
          acceptor_(service_) {
        using namespace boost::asio::ip;

        signals_.add(SIGINT);
        signals_.add(SIGTERM);
        #warning is the following needed?
        // signals_.add(SIGQUIT);
        signals_.async_wait([this] (const boost::system::error_code&, int) {
            XINFO << "stopping xProxy...";
            acceptor_.close();
            session_manager_.stop_all();
        });

        tcp::endpoint e(tcp::v4(), port_);
        acceptor_.open(e.protocol());
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
        acceptor_.bind(e);
        acceptor_.listen();
    }

    DEFAULT_DTOR(server);

    void start() {
        start_accept();
    }

private:
    void start_accept() {
        current_session_ = new session(service_);
        conn_ptr client_conn = current_session_->get_connection(session::CLIENT_SIDE);
        acceptor_.async_accept(client_conn->socket(),
                               [this] (const boost::system::error_code& e) {
            if (e) {
                XERROR << "accept error, code: " << e.value()
                       << ", message: " << e.message();
                return;
            }

            XTRACE << "new client connection, id: " << client_conn->id()
                   << ", addr: " << client_conn->socket().remote_endpoint().address()
                   << ", port: " << client_conn->socket().remote_endpoint().port();

            session_manager_.start(current_session_);

            start_accept();
        });
    }

    unsigned short port_;

    boost::asio::io_service service_;
    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;

    session_manager session_manager_;
    session_ptr current_session_;

    MAKE_NONCOPYABLE(server);
};

} // namespace net
} // namespace x

#endif // SERVER_HPP
