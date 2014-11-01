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
        current_client_connection_ = (new client_connection(service_));
        acceptor_.async_accept(current_client_connection_->socket(),
                               [this] (const boost::system::error_code& e) {
            if (e) {
                XERROR << "accept error, code: " << e.value()
                       << ", message: " << e.message();
                return;
            }

            XTRACE << "new client connection, id: " << current_client_connection_->id()
                   << ", addr: " << current_client_connection_->socket().remote_endpoint().address()
                   << ", port: " << current_client_connection_->socket().remote_endpoint().port();

            current_client_connection_->start();

            start_accept();
        });
    }

    unsigned short port_;

    boost::asio::io_service service_;
    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;

    conn_ptr current_client_connection_;

    MAKE_NONCOPYABLE(server);
};

} // namespace net
} // namespace x

#endif // SERVER_HPP
