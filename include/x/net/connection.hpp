#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <boost/asio.hpp>
#include "x/codec/message_decoder.hpp"
#include "x/codec/message_encoder.hpp"
#include "x/memory/byte_buffer.hpp"
#include "x/message/message.hpp"
#include "x/net/connection_context.hpp"
#include "x/net/socket_wrapper.hpp"
#include "x/util/counter.hpp"
#include "x/util/timer.hpp"

namespace x {
namespace net {

class connection_manager;

class connection : public util::counter<connection>,
                   public std::enable_shared_from_this<connection> {
public:
    connection(context_ptr ctx, connection_manager& mgr);

    virtual ~connection() {
        XDEBUG_WITH_ID(this) << "connection destructed.";
    }

    virtual bool keep_alive() = 0;

    virtual void start() = 0;
    virtual void connect() = 0;
    virtual void handshake(ssl::certificate ca = ssl::certificate(), DH *dh = nullptr) = 0;

    virtual void read();
    virtual void write();
    virtual void write(const message::message& message);
    virtual void reset();

    virtual void on_connect(const boost::system::error_code& e, boost::asio::ip::tcp::resolver::iterator it) = 0;
    virtual void on_read(const boost::system::error_code& e, const char *data, std::size_t length) = 0;
    virtual void on_write() = 0;
    virtual void on_handshake(const boost::system::error_code& e) = 0;

    void stop(bool notify = true);

    void detach() {
        manager_ = nullptr;
    }

    socket_wrapper::socket_type& socket() const {
        return socket_->socket();
    }

    context_ptr get_context() const {
        return context_;
    }

    message::message& get_message() {
        return *message_;
    }

    const message::message& get_message() const {
        return *message_;
    }

    void set_host(const std::string& host) {
        host_ = host;
    }

    std::string get_host() const {
        return host_;
    }

    void set_port(unsigned short port) {
        port_ = port;
    }

    unsigned short get_port() const {
        return port_;
    }

protected:
    void cancel_timer() {
        XDEBUG_WITH_ID(this) << "cancel running timer.";
        timer_.cancel();
    }

    bool connected_;
    bool stopped_;
    std::string host_;
    unsigned short port_;
    std::unique_ptr<socket_wrapper> socket_;
    util::timer timer_;
    context_ptr context_;

    std::unique_ptr<codec::message_decoder> decoder_;
    std::unique_ptr<codec::message_encoder> encoder_;
    std::unique_ptr<message::message> message_;
    connection_manager *manager_;

private:
    void do_write();
    void on_write(const boost::system::error_code& e, std::size_t length);

    enum { FIXED_BUFFER_SIZE = 8192 };

    std::array<char, FIXED_BUFFER_SIZE> buffer_in_;
    std::list<memory::buffer_ptr> buffer_out_;
    bool writing_;
};

typedef std::shared_ptr<connection> connection_ptr;

} // namespace net
} // namespace x

#endif // CONNECTION_HPP
