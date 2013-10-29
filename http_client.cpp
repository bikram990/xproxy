#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "http_client.h"
#include "log.h"

HttpClient::HttpClient(boost::asio::io_service& service,
                       HttpRequest::Ptr request,
                       HttpResponse::Ptr response,
                       boost::asio::ssl::context *context)
    : service_(service), resolver_(service), is_ssl_(context ? true : false),
      request_(request), response_(response), host_(request->host()), port_(request->port()) {
    TRACE_THIS_PTR;
    if(context) {
        ssl_socket_.reset(new ssl_socket(service_, *context));
        ssl_socket_->set_verify_mode(boost::asio::ssl::verify_peer);
        ssl_socket_->set_verify_callback(boost::bind(&HttpClient::VerifyCertificate, this, _1, _2));
    } else {
        socket_.reset(new socket(service_));
    }
}

HttpClient::~HttpClient() {
    TRACE_THIS_PTR;
}

void HttpClient::AsyncSendRequest(RequestHandler::handler_type handler) {
    handler_ = handler;
    ResolveRemote();
}

void HttpClient::ResolveRemote() {
    XDEBUG << "Resolving remote address, host: " << host_ << ", port: " << port_;

    // TODO cache the DNS query result
    boost::asio::ip::tcp::resolver::query query(host_, boost::lexical_cast<std::string>(port_));
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query);

    XDEBUG << "Connecting to remote address: " << endpoint_iterator->endpoint().address();

    boost::asio::async_connect(is_ssl_ ? ssl_socket_->lowest_layer() : *socket_,
                               endpoint_iterator,
                               boost::bind(&HttpClient::OnRemoteConnected,
                                           this,
                                           boost::asio::placeholders::error));
}

void HttpClient::OnRemoteConnected(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to connect to remote server, message: " << e.message();
        handler_(e);
        return;
    }

    XDEBUG << "Dump request before sending(size: " << request_->OutboundBuffer().size() << "):\n"
           << "--------------------------------------------\n"
           << boost::asio::buffer_cast<const char *>(request_->OutboundBuffer().data())
           << "\n--------------------------------------------";

    if(is_ssl_) {
        ssl_socket_->async_handshake(boost::asio::ssl::stream_base::client,
                                     boost::bind(&HttpClient::OnRemoteHandshaken,
                                                 this,
                                                 boost::asio::placeholders::error));
    } else {
        boost::asio::async_write(*socket_, request_->OutboundBuffer(),
                                 boost::bind(&HttpClient::OnRemoteDataSent,
                                             this,
                                             boost::asio::placeholders::error));
    }
}

void HttpClient::OnRemoteHandshaken(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed at handshake step, message: " << e.message();
        handler_(e);
        return;
    }

    boost::asio::async_write(*ssl_socket_, request_->OutboundBuffer(),
                             boost::bind(&HttpClient::OnRemoteDataSent,
                                         this,
                                         boost::asio::placeholders::error));
}

void HttpClient::OnRemoteDataSent(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to write request to remote server, message: " << e.message();
        handler_(e);
        return;
    }
    if(is_ssl_) {
        boost::asio::async_read_until(*ssl_socket_, remote_buffer_, "\r\n",
                                      boost::bind(&HttpClient::OnRemoteStatusLineReceived,
                                                  this,
                                                  boost::asio::placeholders::error));
    } else {
        boost::asio::async_read_until(*socket_, remote_buffer_, "\r\n",
                                      boost::bind(&HttpClient::OnRemoteStatusLineReceived,
                                                  this,
                                                  boost::asio::placeholders::error));
    }
}

void HttpClient::OnRemoteStatusLineReceived(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to read status line from remote server, message: " << e.message();
        handler_(e);
        return;
    }

    // As async_read_until may return more data beyond the delimiter, so we only process the status line
    std::istream response(&remote_buffer_);
    std::getline(response, response_->status_line());
    response_->status_line().erase(response_->status_line().size() - 1); // remove the last \r

    XTRACE << "Status line from remote server: " << response_->status_line();

    if(is_ssl_) {
        boost::asio::async_read_until(*ssl_socket_, remote_buffer_, "\r\n\r\n",
                                      boost::bind(&HttpClient::OnRemoteHeadersReceived,
                                                  this,
                                                  boost::asio::placeholders::error));
    } else {
        boost::asio::async_read_until(*socket_, remote_buffer_, "\r\n\r\n",
                                      boost::bind(&HttpClient::OnRemoteHeadersReceived,
                                                  this,
                                                  boost::asio::placeholders::error));
    }
}

void HttpClient::OnRemoteHeadersReceived(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to read response header from remote server, message: " << e.message();
        handler_(e);
        return;
    }

    XTRACE << "Headers from remote server: \n" << boost::asio::buffer_cast<const char *>(remote_buffer_.data());

    std::istream response(&remote_buffer_);
    std::string header;
    std::size_t body_len = 0;
    bool chunked_encoding = false;
    while(std::getline(response, header)) {
        if(header == "\r") { // there is no more headers
            XDEBUG << "no more headers";
            break;
        }

        std::string::size_type sep_idx = header.find(':');
        if(sep_idx == std::string::npos) {
            XWARN << "Invalid header: " << header;
            continue;
        }

        std::string name = header.substr(0, sep_idx);
        std::string value = header.substr(sep_idx + 1, header.length() - 1 - name.length() - 1); // remove the last \r
        boost::algorithm::trim(name);
        boost::algorithm::trim(value);
        response_->AddHeader(name, value);

        XTRACE << "header name: " << name << ", value: " << value;

        if(name == "Transfer-Encoding") {
            XINFO << "Transfer-Encoding header is found, value: " << value;
            if(value == "chunked")
                chunked_encoding = true;
        }

        if(name == "Content-Length") {
            if(chunked_encoding)
                XWARN << "Both Transfer-Encoding and Content-Length headers are found";
            boost::algorithm::trim(value);
            body_len = boost::lexical_cast<std::size_t>(value);
        }
    }

    if(chunked_encoding) {
        if(is_ssl_) {
            boost::asio::async_read(*ssl_socket_, remote_buffer_,
                                    boost::asio::transfer_at_least(1),
                                    boost::bind(&HttpClient::OnRemoteChunksReceived,
                                                this,
                                                boost::asio::placeholders::error));
        } else {
            boost::asio::async_read(*socket_, remote_buffer_,
                                    boost::asio::transfer_at_least(1),
                                    boost::bind(&HttpClient::OnRemoteChunksReceived,
                                                this,
                                                boost::asio::placeholders::error));
        }
        return;
    }

    if(body_len <= 0) {
        XDEBUG << "This response seems have no body.";
        handler_(e);
        // TODO do something here
        //boost::system::error_code ec;
        //remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        return;
    }

    response_->body_lenth(body_len);
    if(is_ssl_) {
        boost::asio::async_read(*ssl_socket_, remote_buffer_,
                                boost::asio::transfer_at_least(1),
                                boost::bind(&HttpClient::OnRemoteBodyReceived,
                                            this,
                                            boost::asio::placeholders::error));
    } else {
        boost::asio::async_read(*socket_, remote_buffer_,
                                boost::asio::transfer_at_least(1),
                                boost::bind(&HttpClient::OnRemoteBodyReceived,
                                            this,
                                            boost::asio::placeholders::error));
    }
}

void HttpClient::OnRemoteChunksReceived(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to read chunk from remote server, message: " << e.message();
        handler_(e);
        return;
    }

    std::size_t read = remote_buffer_.size();
    boost::asio::streambuf::mutable_buffers_type buf = response_->body().prepare(read);
    std::size_t copied = boost::asio::buffer_copy(buf, remote_buffer_.data());

    XTRACE << "Chunk from remote server, size: " << read
           << ", body copied from raw stream to response, copied: " << copied;

    if(copied < read) {
        // copied should always equal to read, so here just output an error log
        XERROR << "The copied size(" << copied << ") is less than read size(" << read
               << "), but this should never happen.";
    }

    response_->body().commit(copied);
    remote_buffer_.consume(copied);

    bool finished = false;
    const char *body = boost::asio::buffer_cast<const char *>(response_->body().data());
    std::size_t size = response_->body().size();
    if(body[size - 4] == '\r'
       && body[size -3] == '\n'
       && body[size - 2] == '\r'
       && body[size - 1] == '\n') {
        XTRACE << "The end of all chunks has been reached.";
        response_->body_lenth(size);
        finished = true;
    }

    if(!finished) {
        if(is_ssl_) {
            boost::asio::async_read(*ssl_socket_, remote_buffer_,
                                    boost::asio::transfer_at_least(1),
                                    boost::bind(&HttpClient::OnRemoteChunksReceived,
                                                this,
                                                boost::asio::placeholders::error));
        } else {
            boost::asio::async_read(*socket_, remote_buffer_,
                                    boost::asio::transfer_at_least(1),
                                    boost::bind(&HttpClient::OnRemoteChunksReceived,
                                                this,
                                                boost::asio::placeholders::error));
        }
    } else {
        // TODO do something here
        handler_(e);
        //boost::system::error_code ec;
        //remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    }
}

void HttpClient::OnRemoteBodyReceived(const boost::system::error_code& e) {
    if(e) {
        if(e == boost::asio::error::eof)
            XDEBUG << "The remote peer closed the connection.";
        else {
            XWARN << "Failed to read body from remote server, message: " << e.message();
            handler_(e);
            return;
        }
    }

    std::size_t read = remote_buffer_.size();
    boost::asio::streambuf::mutable_buffers_type buf = response_->body().prepare(read);
    std::size_t copied = boost::asio::buffer_copy(buf, remote_buffer_.data());

    XTRACE << "Body from remote server, size: " << read
           << ", body copied from raw stream to response, copied: " << copied
           << ", current body size: " << response_->body().size() + copied
           << ", desired body size: " << response_->body_length();

    if(copied < read) {
        // copied should always equal to read, so here just output an error log
        XERROR << "The copied size(" << copied << ") is less than read size(" << read
               << "), but this should never happen.";
    }

    response_->body().commit(copied);
    remote_buffer_.consume(copied);

    if(response_->body().size() < response_->body_length()) { // there is more content
        if(is_ssl_) {
            boost::asio::async_read(*ssl_socket_, remote_buffer_,
                                    boost::asio::transfer_at_least(1),
                                    boost::bind(&HttpClient::OnRemoteBodyReceived,
                                                this,
                                                boost::asio::placeholders::error));
        } else {
            boost::asio::async_read(*socket_, remote_buffer_,
                                    boost::asio::transfer_at_least(1),
                                    boost::bind(&HttpClient::OnRemoteBodyReceived,
                                                this,
                                                boost::asio::placeholders::error));
        }
    } else {
        // TODO do something here
        handler_(e);
        //boost::system::error_code ec;
        //remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    }
}

bool HttpClient::VerifyCertificate(bool pre_verified, boost::asio::ssl::verify_context& ctx) {
    // TODO enhance this function
    char subject_name[256];
    X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    XDEBUG << "Verify remote certificate, subject name: " << subject_name
           << ", pre_verified value: " << pre_verified;

    return true;
}
