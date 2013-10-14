#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "https_proxy_handler.h"
#include "http_proxy_session.h"
#include "log.h"
#include "proxy_configuration.h"

extern ProxyConfiguration g_config;

HttpsProxyHandler::HttpsProxyHandler(HttpProxySession& session,
                                     HttpRequestPtr request)
    : session_(session), local_socket_(session.LocalSocket()),
      context_(boost::asio::ssl::context::sslv23),
      remote_socket_(session.service(), context_),
      resolver_(session.service()),
      origin_request_(request) {
    TRACE_THIS_PTR;
    remote_socket_.set_verify_mode(boost::asio::ssl::verify_peer);
    remote_socket_.set_verify_callback(boost::bind(&HttpsProxyHandler::VerifyCertificate, this, _1, _2));
}

HttpsProxyHandler::~HttpsProxyHandler() {
    TRACE_THIS_PTR;
}

void HttpsProxyHandler::HandleRequest() {
    XTRACE << "New request received, host: " << origin_request_->host()
           << ", port: " << origin_request_->port();
    ResolveRemote();
}

void HttpsProxyHandler::BuildProxyRequest(HttpRequest& request) {
    // TODO improve this, refine the headers
    boost::asio::streambuf& origin_body_buf = origin_request_->OutboundBuffer();
    boost::asio::streambuf::const_buffers_type origin_body = origin_body_buf.data();
    std::size_t length = origin_body_buf.size();

    request.method("POST").uri("/proxy").major_version(1).minor_version(1)
           .AddHeader("Host", g_config.GetGAEAppId() + ".appspot.com")
           .AddHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:24.0) Gecko/20100101 Firefox/24.0")
           .AddHeader("Connection", "close")
           .AddHeader("Content-Length", boost::lexical_cast<std::string>(length))
           .body(boost::asio::buffers_begin(origin_body),
                 boost::asio::buffers_end(origin_body))
           .body_length(length);
}

void HttpsProxyHandler::ResolveRemote() {
    const std::string host = g_config.GetGAEServerDomain();
    short port = 443;

    XDEBUG << "Resolving remote address, host: " << host << ", port: " << port;

    boost::asio::ip::tcp::resolver::query query(host, boost::lexical_cast<std::string>(port));
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query);

    XDEBUG << "Connecting to remote address: " << endpoint_iterator->endpoint().address();

    boost::asio::async_connect(remote_socket_.lowest_layer(), endpoint_iterator,
                               boost::bind(&HttpsProxyHandler::OnRemoteConnected,
                                           this,
                                           boost::asio::placeholders::error));
}

void HttpsProxyHandler::OnRemoteConnected(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to connect to remote server, message: " << e.message();
        session_.Stop();
        return;
    }

    remote_socket_.async_handshake(boost::asio::ssl::stream_base::client,
                                   boost::bind(&HttpsProxyHandler::OnRemoteHandshaken,
                                               this,
                                               boost::asio::placeholders::error));
}

void HttpsProxyHandler::OnRemoteDataSent(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to write request to remote server, message: " << e.message();
        session_.Stop();
        return;
    }
    boost::asio::async_read_until(remote_socket_, remote_buffer_, "\r\n",
                                  boost::bind(&HttpsProxyHandler::OnRemoteStatusLineReceived,
                                              this,
                                              boost::asio::placeholders::error));
}

void HttpsProxyHandler::OnRemoteStatusLineReceived(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to read status line from remote server, message: " << e.message();
        session_.Stop();
        return;
    }

    // As async_read_until may return more data beyond the delimiter, so we only process the status line
    std::istream response(&remote_buffer_);
    std::getline(response, response_.status_line());
    response_.status_line() += '\n'; // append the missing newline character

    XDEBUG << "Status line from remote server: " << response_.status_line();

    boost::asio::async_read_until(remote_socket_, remote_buffer_, "\r\n\r\n",
                                  boost::bind(&HttpsProxyHandler::OnRemoteHeadersReceived,
                                              this,
                                              boost::asio::placeholders::error));
}

void HttpsProxyHandler::OnRemoteHeadersReceived(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to read response header from remote server, message: " << e.message();
        session_.Stop();
        return;
    }

    XDEBUG << "Headers from remote server: \n" << boost::asio::buffer_cast<const char *>(remote_buffer_.data());

    std::istream response(&remote_buffer_);
    std::string header;
    std::size_t body_len = 0;
    while(std::getline(response, header)) {
        if(header == "\r") { // there is no more headers
            XDEBUG << "no more headers";
            break;
        }

        std::string::size_type sep_idx = header.find(": ");
        if(sep_idx == std::string::npos) {
            XWARN << "Invalid header: " << header;
            continue;
        }

        std::string name = header.substr(0, sep_idx);
        std::string value = header.substr(sep_idx + 2, header.length() - 1 - name.length() - 2); // remove the last \r
        response_.AddHeader(name, value);

        XTRACE << "header name: " << name << ", value: " << value;

        if(name == "Content-Length") {
            boost::algorithm::trim(value);
            body_len = boost::lexical_cast<std::size_t>(value);
        }
    }

    if(body_len <= 0) {
        XERROR << "The proxy server returns no body.";
        //boost::system::error_code ec;
        //remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        return;
    }

    response_.body_lenth(body_len);
    boost::asio::async_read(remote_socket_, remote_buffer_, boost::asio::transfer_at_least(1),
                            boost::bind(&HttpsProxyHandler::OnRemoteBodyReceived,
                                        this,
                                        boost::asio::placeholders::error));
}

void HttpsProxyHandler::OnRemoteBodyReceived(const boost::system::error_code& e) {
    if(e) {
        if(e == boost::asio::error::eof)
            XDEBUG << "The remote peer closed the connection.";
        else {
            XWARN << "Failed to read body from remote server, message: " << e.message();
            session_.Stop();
            return;
        }
    }

    std::size_t read = remote_buffer_.size();
    std::size_t copied = boost::asio::buffer_copy(boost::asio::buffer(response_.body()), remote_buffer_.data());

    XDEBUG << "Body from remote server, size: " << read
           << ", content:\n" << boost::asio::buffer_cast<const char *>(remote_buffer_.data());
    XDEBUG << "Body copied from raw stream to response, copied: " << copied
           << ", response body size: " << response_.body().size();

    boost::asio::async_write(local_socket_, boost::asio::buffer(response_.body(), copied),
                             boost::bind(&HttpsProxyHandler::OnLocalDataSent,
                                         this,
                                         boost::asio::placeholders::error, read >= response_.body_length()));
    if(copied < read) {
        // TODO the response's body buffer is less than read content, try write again
    }

    remote_buffer_.consume(read); // the read bytes are consumed

    if(read < response_.body_length()) { // there is more content
        response_.body_lenth(response_.body_length() - read);
        boost::asio::async_read(remote_socket_, remote_buffer_, boost::asio::transfer_at_least(1/*body_len*/),
                                boost::bind(&HttpsProxyHandler::OnRemoteBodyReceived,
                                            this,
                                            boost::asio::placeholders::error));
    } else {
        //boost::system::error_code ec;
        //remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    }
}

void HttpsProxyHandler::OnLocalDataSent(const boost::system::error_code& e,
                                        bool finished) {
    if(e) {
        XWARN << "Failed to write response to local socket, message: " << e.message();
        session_.Stop();
        return;
    }

    XDEBUG << "Content written to local socket.";

    if(!finished)
        return;

    if(!e) {
        boost::system::error_code ec;
        local_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        //remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    }

    if(e != boost::asio::error::operation_aborted) {
        //manager_.Stop(shared_from_this());
        session_.Terminate();
    }
}

bool HttpsProxyHandler::VerifyCertificate(bool pre_verified, boost::asio::ssl::verify_context& ctx) {
    // TODO enhance this function
    char subject_name[256];
    X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    XDEBUG << "Verify remote certificate, subject name: " << subject_name
           << ", pre_verified value: " << pre_verified;

    return true;
}

void HttpsProxyHandler::OnRemoteHandshaken(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed at handshake step, message: " << e.message();
        session_.Stop();
        return;
    }

    BuildProxyRequest(proxy_request_);

    XTRACE << boost::asio::buffer_cast<const char *>(proxy_request_.OutboundBuffer().data());

    boost::asio::async_write(remote_socket_, proxy_request_.OutboundBuffer(),
                             boost::bind(&HttpsProxyHandler::OnRemoteDataSent,
                                         this,
                                         boost::asio::placeholders::error));
}
