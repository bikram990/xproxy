#ifndef SESSION_H
#define SESSION_H

#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "filter_chain.h"
#include "http_container.h"
#include "http_request_decoder.h"
#include "http_response_decoder.h"
#include "resource_manager.h"
#include "session_manager.h"
#include "socket.h"

struct SessionContext {
    SessionContext() :
        request(new HttpContainer),
        response(new HttpContainer),
        https(false) {}

    ~SessionContext() {
        delete request;
        delete response;
    }

    HttpContainer *request;
    HttpContainer *response;
    std::string host;
    unsigned short port;
    bool https;
};

class Session : public Resettable,
                public boost::enable_shared_from_this<Session>,
                private boost::noncopyable {
public:
    typedef Session this_type;

    enum {
        kDefaultClientInBufferSize = 8192,
        kDefaultServerInBufferSize = 8192,  // TODO decide the proper value
        kDefaultClientTimeoutValue = 60,    // 60 seconds
        kDefaultServerTimeoutValue = 15     // 15 seconds
    };

    static Session *create(boost::asio::io_service& service,
                           SessionManager& manager) {
        ++counter_;
        return new Session(service, manager);
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
    void stop();

    std::size_t id() const { return id_; }

    void reset() {
        request_decoder_->reset();
        response_decoder_->reset();
        context_.request->reset();
        context_.response->reset();

        finished_ = false;
        /// the following should not be reset
        // host_.clear();
        // port_ = 0;
        // server_connected_ = false;
        // https_ = false;
        // reused_ = false;

#define CLEAR_STREAMBUF(buf) if(buf.size() > 0) buf.consume(buf.size())

        CLEAR_STREAMBUF(client_in_);
        CLEAR_STREAMBUF(client_out_);
        CLEAR_STREAMBUF(server_in_);
        CLEAR_STREAMBUF(server_out_);

#undef CLEAR_STREAMBUF
    }

    void AsyncReadFromClient();
    void AsyncWriteSSLReplyToClient();
    void AsyncConnectToServer();
    void AsyncWriteToServer();
    void AsyncReadFromServer();
    void AsyncWriteToClient();

    virtual ~Session() {
        XDEBUG_WITH_ID << "Destructor called.";

        if(client_socket_) delete client_socket_;
        if(server_socket_) delete server_socket_;
        if(chain_) delete chain_;
        if(request_decoder_) delete request_decoder_;
        if(response_decoder_) delete response_decoder_;
    }

private:
    Session(boost::asio::io_service& service, SessionManager& manager)
        : id_(counter_),
          manager_(manager),
          service_(service),
          client_socket_(Socket::Create(service)),
          server_socket_(Socket::Create(service)),
          client_timer_(service),
          server_timer_(service),
          resolver_(service),
          chain_(new FilterChain),
          request_decoder_(new HttpRequestDecoder),
          response_decoder_(new HttpResponseDecoder),
          server_connected_(false),
          finished_(false),
          reused_(false) {}

private:
    void OnClientDataReceived(const boost::system::error_code& e);
    void OnClientSSLReplySent(const boost::system::error_code& e);
    void OnServerConnected(const boost::system::error_code& e);
    void OnServerDataSent(const boost::system::error_code& e);
    void OnServerDataReceived(const boost::system::error_code& e);
    void OnClientDataSent(const boost::system::error_code& e);
    void OnServerTimeout(const boost::system::error_code& e);
    void OnClientTimeout(const boost::system::error_code& e);

private:
    void InitClientSSLContext() {
        auto ca = ResourceManager::GetCertManager().GetCertificate(context_.host);
        auto dh = ResourceManager::GetCertManager().GetDHParameters();
        client_socket_->SwitchProtocol(kHttps, kServer, ca, dh);
    }

private:
    static boost::atomic<std::size_t> counter_;

private:
    std::size_t id_;

    SessionManager& manager_;

    boost::asio::io_service& service_;
    Socket *client_socket_;
    Socket *server_socket_;

    boost::asio::deadline_timer client_timer_;
    boost::asio::deadline_timer server_timer_;

    boost::asio::ip::tcp::resolver resolver_;

    FilterChain *chain_;

    Decoder *request_decoder_;
    Decoder *response_decoder_;

    SessionContext context_;

    bool server_connected_;
    bool finished_;
    bool reused_;

    boost::asio::streambuf client_in_;
    boost::asio::streambuf client_out_;
    boost::asio::streambuf server_in_;
    boost::asio::streambuf server_out_;
};

#endif // SESSION_H
