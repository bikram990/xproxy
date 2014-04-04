#ifndef CONNECTION_H
#define CONNECTION_H

#include <memory>
#include <mutex>
#include "common.h"
#include "socket.h"

class HttpMessage;
class Session;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    void read();
    void write(std::shared_ptr<HttpMessage> message);

    template<typename Data, typename Converter>
    void write(Data& data, const Converter& convert) {
        std::lock_guard<std::mutex> lock(lock_);
        bool owner = out_->size() <= 0;
        auto buf = owner ? out_.get() : aux_out_.get();
        if (!convert(data, *buf)) {
            XERROR << "Error occurred during serialization.";
            DestroySession();
            return;
        }

        const static std::string msg1("write() called, will start a write operation.");
        const static std::string msg2("write() called, a write operation exists, just write data to out buffer.");
        XDEBUG << (owner ? msg1 : msg2);

        if (owner)
            write();
    }

    boost::asio::io_service& service() { return service_; }

    socket_type& socket() { return socket_->socket(); }

    const std::string& host() const { return host_; }

    unsigned short port() const { return port_; }

    void host(const std::string& host) { host_ = host; }

    void port(unsigned short port) { port_ = port; }

    // in class ServerConnection, this method should be overridden
    virtual void OnHeadersComplete() {}

    // in class ServerConnection, this method should be overridden
    virtual void OnBody() {}

    // this method should be overridden in both derived classes
    virtual void OnBodyComplete() {}

    // this method will be called when a round of communication completed
    // that is to say, all response data has been received and sent to client
    // both derived classes should overridden this method, to clear and
    // reset the connection waiting for next round of communication
    virtual void OnMessageExchangeComplete() = 0;

protected:
    Connection(std::shared_ptr<Session> session,
               long timeout = 30,
               std::size_t buffer_size = 8192);
    virtual ~Connection() = default;

    virtual void init() = 0;

    virtual void connect() = 0;

    virtual void OnRead(const boost::system::error_code& e, std::size_t length) = 0;
    virtual void OnWritten(const boost::system::error_code& e, std::size_t length) = 0;
    virtual void OnTimeout(const boost::system::error_code& e) = 0;

    virtual void ConstructMessage();

    void write();

    void reset();

    // some helper functions
    bool SessionInvalidated();
    void DestroySession();
    void StartTimer();
    void CancelTimer();

protected:
    std::weak_ptr<Session> session_;

    boost::asio::io_service& service_;

    boost::asio::deadline_timer timer_;
    boost::posix_time::seconds timeout_;
    bool timer_running_;
    bool timer_triggered_;

    std::unique_ptr<Socket> socket_;
    bool connected_;

    std::size_t buffer_size_;
    boost::asio::streambuf buffer_in_;

    std::unique_ptr<boost::asio::streambuf> out_;
    std::unique_ptr<boost::asio::streambuf> aux_out_;
    std::mutex lock_; // lock for out buffer

    std::shared_ptr<HttpMessage> message_;

    std::string host_;
    unsigned short port_;

private:
    DISABLE_COPY_AND_ASSIGNMENT(Connection);
};

#endif // CONNECTION_H
