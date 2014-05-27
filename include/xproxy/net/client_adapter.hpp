#ifndef CLIENT_ADAPTER_HPP
#define CLIENT_ADAPTER_HPP

#include "xproxy/message/http/http_parser.hpp"
#include "xproxy/net/connection.hpp"
#include "xproxy/plugin/plugin_manager.hpp"

namespace xproxy {
namespace net {

class ClientAdapter : public ConnectionAdapter, public message::http::HttpParserObserver {
public:
    enum { kDefaultClientTimeout = 30 }; // 30 seconds

    ClientAdapter(Connection& connection, std::shared_ptr<plugin::PluginChain> chain);
    DEFAULT_VIRTUAL_DTOR(ClientAdapter);

    virtual void onConnect(const boost::system::error_code& e);
    virtual void onHandshake(const boost::system::error_code& e);
    virtual void onRead(const boost::system::error_code& e, const char *data, std::size_t length);
    virtual void onWrite(const boost::system::error_code& e);
    virtual void onTimeout(const boost::system::error_code& e);

    virtual void onHeadersComplete(message::http::HttpMessage& message);
    virtual void onBody(message::http::HttpMessage& message);
    virtual void onMessageComplete(message::http::HttpMessage& message);

private:
    bool ParseRemotePeer(std::string& remote_host, std::string& remote_port);

private:
    Connection& connection_;
    std::unique_ptr<message::http::HttpMessage> message_;
    std::unique_ptr<message::http::HttpParser> parser_;
    std::shared_ptr<plugin::PluginChain> chain_;
    std::shared_ptr<memory::ByteBuffer> cache_;
};

} // namespace net
} // namespace xproxy

#endif // CLIENT_ADAPTER_HPP
