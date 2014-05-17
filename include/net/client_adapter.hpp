#ifndef CLIENT_ADAPTER_HPP
#define CLIENT_ADAPTER_HPP

#include "message/http/http_parser.hpp"
#include "net/connection.hpp"

namespace xproxy {
namespace util { class Timer; }
namespace net {

class ClientAdapter : public ConnectionAdapter, public message::http::HttpParserObserver {
public:
    ClientAdapter(Connection& connection);
    DEFAULT_VIRTUAL_DTOR(ClientAdapter);

    virtual void onConnect(const boost::system::error_code& e);
    virtual void onHandshake(const boost::system::error_code& e);
    virtual void onRead(const boost::system::error_code& e, const char *data, std::size_t length);
    virtual void onWrite(const boost::system::error_code& e);

    virtual void onHeadersComplete(message::http::HttpMessage& message);
    virtual void onBody(message::http::HttpMessage& message);
    virtual void onMessageComplete(message::http::HttpMessage& message);

private:
    bool ParseRemotePeer();

private:
    Connection& connection_;
    util::Timer timer_;
    std::unique_ptr<message::http::HttpMessage> message_;
    std::unique_ptr<message::http::HttpParser> parser_;

    bool https_;
    bool ssl_built_;
    std::string remote_host_;
    std::string remote_port_;
};

} // namespace net
} // namespace xproxy

#endif // CLIENT_ADAPTER_HPP
