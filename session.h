#ifndef SESSION_H
#define SESSION_H

#include <boost/asio.hpp>
#include "socket.h"

class Session : private boost::noncopyable {
public:
    static Session *create(const boost::asio::io_service& service) {
        // TODO add counter feature here later
        return new Session(service);
    }

    boost::asio::ip::tcp::socket& ClientSocket() const {
        assert(client_socket_ != nullptr);
        return client_socket_->socket();
    }

    boost::asio::ip::tcp::socket& ServerSocket() const {
        assert(server_socket_ != nullptr);
        return server_socket_->socket();
    }

    void start();

    virtual ~Session() {
        if(client_socket_) delete client_socket_;
        if(server_socket_) delete server_socket_;
    }

private:
    Session(const boost::asio::io_service& service)
        : service_(service), client_socket_(Socket::Create(service)),
          server_socket_(Socket::Create(service)) {}

private:
    boost::asio::io_service& service_;
    Socket *client_socket_;
    Socket *server_socket_;
};

#endif // SESSION_H
