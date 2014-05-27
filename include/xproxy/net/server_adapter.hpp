#ifndef SERVER_ADAPTER_HPP
#define SERVER_ADAPTER_HPP

#include "xproxy/message/http/http_parser.hpp"
#include "xproxy/net/connection.hpp"
#include "xproxy/plugin/plugin_manager.hpp"

namespace xproxy {
namespace net {

class ServerAdapter : public ConnectionAdapter, public message::http::HttpParserObserver {
public:
    enum { kDefaultServerTimeout = 15 }; // 15 seconds

    ServerAdapter(Connection& connection, plugin::PluginChain *chain);
    DEFAULT_VIRTUAL_DTOR(ServerAdapter);

    virtual void onConnect(const boost::system::error_code& e);
    virtual void onHandshake(const boost::system::error_code& e);
    virtual void onRead(const boost::system::error_code& e, const char *data, std::size_t length);
    virtual void onWrite(const boost::system::error_code& e);
    virtual void onTimeout(const boost::system::error_code& e);

    virtual void onHeadersComplete(message::http::HttpMessage& message);
    virtual void onBody(message::http::HttpMessage& message);
    virtual void onMessageComplete(message::http::HttpMessage& message);

private:
    void flushCache();

private:
    Connection& connection_;
    std::unique_ptr<message::http::HttpMessage> message_;
    std::unique_ptr<message::http::HttpParser> parser_;
    std::unique_ptr<plugin::PluginChain> chain_;
    std::shared_ptr<memory::ByteBuffer> cache_;
};

} // namespace net
} // namespace xproxy
#endif // SERVER_ADAPTER_HPP
