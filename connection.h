#ifndef CONNECTION_H
#define CONNECTION_H

#include <memory>
#include "common.h"
#include "socket.h"

class Session;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    virtual void read();
    virtual void write();


    boost::asio::streambuf& InboundBuffer() { return buffer_in_; }
    boost::asio::streambuf& OutboundBuffer() { return buffer_out_; }

protected:
    Connection(std::shared_ptr<Session> session,
               long timeout = 30,
               std::size_t buffer_size = 8192); // TODO
    virtual ~Connection();

    virtual void OnRead(const boost::system::error_code& e) = 0;
    virtual void OnWritten(const boost::system::error_code& e) = 0;

    virtual void ConstructMessage() = 0;

protected:
    std::weak_ptr<Session> session_;

    boost::asio::io_service& service_;
    boost::asio::deadline_timer timer_;
    boost::posix_time::seconds timeout_;

    std::unique_ptr<Socket> socket_;
    bool connected_;

    std::size_t buffer_size_;
    boost::asio::streambuf buffer_in_;
    boost::asio::streambuf buffer_out_;

private:
    DISABLE_COPY_AND_ASSIGNMENT(Connection);
};

#endif // CONNECTION_H
