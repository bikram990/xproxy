#ifndef CONNECTION_H
#define CONNECTION_H

#include <memory>
#include "common.h"
#include "socket.h"

class HttpMessage;
class Session;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    typedef std::function<void(std::shared_ptr<HttpMessage>)> notify_callback;

    virtual void read();
    virtual void write();

    void init();

protected:
    Connection(std::shared_ptr<Session> session,
               notify_callback&& complete_callback,
               notify_callback&& new_data_callback = notify_callback(),
               long timeout = 30,
               std::size_t buffer_size = 8192); // TODO
    virtual ~Connection();

    virtual void OnRead(const boost::system::error_code& e) = 0;
    virtual void OnWritten(const boost::system::error_code& e) = 0;
    virtual void OnTimeout(const boost::system::error_code& e) = 0;

    virtual void InitMessage() = 0;
    virtual void ConstructMessage();

    void reset();

protected:
    std::weak_ptr<Session> session_;

    boost::asio::io_service& service_;

    boost::asio::deadline_timer timer_;
    boost::posix_time::seconds timeout_;
    bool timer_triggered_;

    std::unique_ptr<Socket> socket_;
    bool connected_;

    std::size_t buffer_size_;
    boost::asio::streambuf buffer_in_;
    boost::asio::streambuf buffer_out_;

    std::shared_ptr<HttpMessage> message_;

    notify_callback new_data_callback_; // when new data come
    notify_callback complete_callback_; // when message is completed

private:
    DISABLE_COPY_AND_ASSIGNMENT(Connection);
};

#endif // CONNECTION_H
