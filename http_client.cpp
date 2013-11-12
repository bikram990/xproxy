#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "http_client.h"
#include "http_request.h"
#include "http_response.h"
#include "log.h"

boost::atomic<std::size_t> HttpClient::counter_(0);

HttpClient::HttpClient(boost::asio::io_service& service)
    : id_(counter_), state_(kAvailable),
      service_(service), strand_(service), resolver_(service),
      is_ssl_(false), persistent_(false),
      request_(NULL), response_(NULL), host_(), port_(0) {
    TRACE_THIS_PTR_WITH_ID;
}

HttpClient::~HttpClient() {
    TRACE_THIS_PTR_WITH_ID;
}

void HttpClient::AsyncSendRequest(HttpProxySession::Mode mode,
                                  HttpRequest *request,
                                  HttpResponse *response,
                                  callback_type callback) {
    if(!callback) {
        XERROR_WITH_ID << "Invalid callback parameter.";
        return;
    }
//    if(state_ != kAvailable) {
//        XERROR_WITH_ID << "This http client is currently in use.";
//        callback(boost::asio::error::already_started);
//        return;
//    }
    if(!request || !response) {
        XERROR_WITH_ID << "Invalid request or response.";
        callback(boost::asio::error::invalid_argument);
        return;
    }

    request_ = request;
    response_ = response;
    callback_ = callback;
    host_ = request->host();
    port_ = request->port();
    state_ = kInUse;

    if(!persistent_) {
        if(mode == HttpProxySession::HTTPS) {
            is_ssl_ = true;
            ssl_context_.reset(new boost::asio::ssl::context(boost::asio::ssl::context::sslv23));
            ssl_socket_.reset(new ssl_socket_type(service_, *ssl_context_));
            ssl_socket_->set_verify_mode(boost::asio::ssl::verify_peer);
            // TODO should the following be wrapped with strand?
            ssl_socket_->set_verify_callback(boost::bind(&HttpClient::VerifyCertificate, this, _1, _2));
        } else {
            is_ssl_ = false;
            socket_.reset(new socket_type(service_));
        }
        ResolveRemote();
    } else {
        if((is_ssl_ && mode != HttpProxySession::HTTPS) ||
                (!is_ssl_ && mode == HttpProxySession::HTTPS)) {
            XERROR_WITH_ID << "Mismatch protocol type.";
            callback(boost::asio::error::operation_not_supported);
            return;
        }
        if(is_ssl_ && !ssl_socket_->lowest_layer().is_open()) {
            XERROR_WITH_ID << "SSL socket is already closed.";
            callback(boost::asio::error::not_connected);
            return;
        }
        if(!is_ssl_ && !socket_->is_open()) {
            XERROR_WITH_ID << "Socket is already closed.";
            callback(boost::asio::error::not_connected);
            return;
        }

        XDEBUG_WITH_ID << "Dump request before sending(size: " << request_->OutboundBuffer().size() << "):\n"
                       << "--------------------------------------------\n"
                       << boost::asio::buffer_cast<const char *>(request_->OutboundBuffer().data())
                       << "\n--------------------------------------------";

        if(is_ssl_) {
            boost::asio::async_write(*ssl_socket_, request_->OutboundBuffer(),
                                     strand_.wrap(boost::bind(&HttpClient::OnRemoteDataSent,
                                                              this,
                                                              boost::asio::placeholders::error)));
        } else {
            boost::asio::async_write(*socket_, request_->OutboundBuffer(),
                                     strand_.wrap(boost::bind(&HttpClient::OnRemoteDataSent,
                                                              this,
                                                              boost::asio::placeholders::error)));
        }
    }

    persistent_ = false; // reset persistent_ to false
}

void HttpClient::ResolveRemote() {
    XDEBUG_WITH_ID << "Resolving remote address, host: " << host_ << ", port: " << port_;

    // TODO cache the DNS query result
    boost::asio::ip::tcp::resolver::query query(host_, boost::lexical_cast<std::string>(port_));
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query);

    XDEBUG_WITH_ID << "Connecting to remote address: " << endpoint_iterator->endpoint().address();

    boost::asio::async_connect(is_ssl_ ? ssl_socket_->lowest_layer() : *socket_,
                               endpoint_iterator,
                               strand_.wrap(boost::bind(&HttpClient::OnRemoteConnected,
                                                        this,
                                                        boost::asio::placeholders::error)));
}

void HttpClient::OnRemoteConnected(const boost::system::error_code& e) {
    if(e) {
        XWARN_WITH_ID << "Failed to connect to remote server, message: " << e.message();
        callback_(e);
        return;
    }

    XDEBUG_WITH_ID << "Dump request before sending(size: " << request_->OutboundBuffer().size() << "):\n"
           << "--------------------------------------------\n"
           << boost::asio::buffer_cast<const char *>(request_->OutboundBuffer().data())
           << "\n--------------------------------------------";

    if(is_ssl_) {
        ssl_socket_->async_handshake(boost::asio::ssl::stream_base::client,
                                     strand_.wrap(boost::bind(&HttpClient::OnRemoteHandshaken,
                                                              this,
                                                              boost::asio::placeholders::error)));
    } else {
        boost::asio::async_write(*socket_, request_->OutboundBuffer(),
                                 strand_.wrap(boost::bind(&HttpClient::OnRemoteDataSent,
                                                          this,
                                                          boost::asio::placeholders::error)));
    }
}

void HttpClient::OnRemoteHandshaken(const boost::system::error_code& e) {
    if(e) {
        XWARN_WITH_ID << "Failed at handshake step, message: " << e.message();
        callback_(e);
        return;
    }

    boost::asio::async_write(*ssl_socket_, request_->OutboundBuffer(),
                             strand_.wrap(boost::bind(&HttpClient::OnRemoteDataSent,
                                                      this,
                                                      boost::asio::placeholders::error)));
}

void HttpClient::OnRemoteDataSent(const boost::system::error_code& e) {
    if(e) {
        XWARN_WITH_ID << "Failed to write request to remote server, message: " << e.message();
        callback_(e);
        return;
    }

    if(is_ssl_) {
        boost::asio::async_read_until(*ssl_socket_, remote_buffer_, "\r\n",
                                      strand_.wrap(boost::bind(&HttpClient::OnRemoteStatusLineReceived,
                                                               this,
                                                               boost::asio::placeholders::error)));
    } else {
        boost::asio::async_read_until(*socket_, remote_buffer_, "\r\n",
                                      strand_.wrap(boost::bind(&HttpClient::OnRemoteStatusLineReceived,
                                                               this,
                                                               boost::asio::placeholders::error)));
    }
}

void HttpClient::OnRemoteStatusLineReceived(const boost::system::error_code& e) {
    if(e) {
        XWARN_WITH_ID << "Failed to read status line from remote server, message: " << e.message();
        callback_(e);
        return;
    }

    // As async_read_until may return more data beyond the delimiter, so we only process the status line
    std::istream response(&remote_buffer_);
    std::getline(response, response_->status_line());
    response_->status_line().erase(response_->status_line().size() - 1); // remove the last \r

    XTRACE_WITH_ID << "Status line from remote server:\n"
           << "--------------------------------------------\n"
           << response_->status_line()
           << "\n--------------------------------------------";

    if(is_ssl_) {
        boost::asio::async_read_until(*ssl_socket_, remote_buffer_, "\r\n\r\n",
                                      strand_.wrap(boost::bind(&HttpClient::OnRemoteHeadersReceived,
                                                               this,
                                                               boost::asio::placeholders::error)));
    } else {
        boost::asio::async_read_until(*socket_, remote_buffer_, "\r\n\r\n",
                                      strand_.wrap(boost::bind(&HttpClient::OnRemoteHeadersReceived,
                                                               this,
                                                               boost::asio::placeholders::error)));
    }
}

void HttpClient::OnRemoteHeadersReceived(const boost::system::error_code& e) {
    if(e) {
        XWARN_WITH_ID << "Failed to read response header from remote server, message: " << e.message();
        callback_(e);
        return;
    }

    XTRACE_WITH_ID << "Headers from remote server:\n"
           << "--------------------------------------------\n"
           << boost::asio::buffer_cast<const char *>(remote_buffer_.data())
           << "\n--------------------------------------------";

    std::istream response(&remote_buffer_);
    std::string header;
    std::size_t body_len = 0;
    bool chunked_encoding = false;
    while(std::getline(response, header)) {
        if(header == "\r") { // there is no more headers
//            XDEBUG_WITH_ID << "no more headers";
            break;
        }

        std::string::size_type sep_idx = header.find(':');
        if(sep_idx == std::string::npos) {
            XWARN_WITH_ID << "Invalid header: " << header;
            continue;
        }

        std::string name = header.substr(0, sep_idx);
        std::string value = header.substr(sep_idx + 1, header.length() - 1 - name.length() - 1); // remove the last \r
        boost::algorithm::trim(name);
        boost::algorithm::trim(value);
        response_->AddHeader(name, value);

//        XTRACE_WITH_ID << "header name: " << name << ", value: " << value;

        if(name == "Transfer-Encoding") {
            XINFO_WITH_ID << "Transfer-Encoding header is found, value: " << value;
            if(value == "chunked")
                chunked_encoding = true;
        }

        if(name == "Content-Length") {
            if(chunked_encoding)
                XWARN_WITH_ID << "Both Transfer-Encoding and Content-Length headers are found";
            boost::algorithm::trim(value);
            body_len = boost::lexical_cast<std::size_t>(value);
        }

        if(name == "Connection" && value == "keep-alive") {
            persistent_ = true;
            XDEBUG_WITH_ID << "This is a persistent connection.";
        }
    }

    if(chunked_encoding) {
        if(is_ssl_) {
            boost::asio::async_read(*ssl_socket_, remote_buffer_,
                                    boost::asio::transfer_at_least(1),
                                    strand_.wrap(boost::bind(&HttpClient::OnRemoteChunksReceived,
                                                             this,
                                                             boost::asio::placeholders::error)));
        } else {
            boost::asio::async_read(*socket_, remote_buffer_,
                                    boost::asio::transfer_at_least(1),
                                    strand_.wrap(boost::bind(&HttpClient::OnRemoteChunksReceived,
                                                             this,
                                                             boost::asio::placeholders::error)));
        }
        return;
    }

    if(body_len <= 0) {
        XDEBUG_WITH_ID << "This response seems have no body.";
        state_ = kAvailable;
        callback_(e);
        // TODO do something here
        //boost::system::error_code ec;
        //remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        return;
    }

    response_->body_lenth(body_len);
    if(is_ssl_) {
        boost::asio::async_read(*ssl_socket_, remote_buffer_,
                                boost::asio::transfer_at_least(1),
                                strand_.wrap(boost::bind(&HttpClient::OnRemoteBodyReceived,
                                                         this,
                                                         boost::asio::placeholders::error)));
    } else {
        boost::asio::async_read(*socket_, remote_buffer_,
                                boost::asio::transfer_at_least(1),
                                strand_.wrap(boost::bind(&HttpClient::OnRemoteBodyReceived,
                                                         this,
                                                         boost::asio::placeholders::error)));
    }
}

void HttpClient::OnRemoteChunksReceived(const boost::system::error_code& e) {
    if(e) {
        XWARN_WITH_ID << "Failed to read chunk from remote server, message: " << e.message();
        callback_(e);
        return;
    }

    std::size_t read = remote_buffer_.size();
    boost::asio::streambuf::mutable_buffers_type buf = response_->body().prepare(read);
    std::size_t copied = boost::asio::buffer_copy(buf, remote_buffer_.data());

    XTRACE_WITH_ID << "Chunk from remote server, size: " << read
           << ", body copied from raw stream to response, copied: " << copied;

    if(copied < read) {
        // copied should always equal to read, so here just output an error log
        XERROR_WITH_ID << "The copied size(" << copied << ") is less than read size(" << read
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
        XTRACE_WITH_ID << "The end of all chunks has been reached.";
        response_->body_lenth(size);
        finished = true;
    }

    if(!finished) {
        if(is_ssl_) {
            boost::asio::async_read(*ssl_socket_, remote_buffer_,
                                    boost::asio::transfer_at_least(1),
                                    strand_.wrap(boost::bind(&HttpClient::OnRemoteChunksReceived,
                                                             this,
                                                             boost::asio::placeholders::error)));
        } else {
            boost::asio::async_read(*socket_, remote_buffer_,
                                    boost::asio::transfer_at_least(1),
                                    strand_.wrap(boost::bind(&HttpClient::OnRemoteChunksReceived,
                                                             this,
                                                             boost::asio::placeholders::error)));
        }
    } else {
        // TODO do something here
        state_ = kAvailable;
        callback_(e);
        //boost::system::error_code ec;
        //remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    }
}

void HttpClient::OnRemoteBodyReceived(const boost::system::error_code& e) {
    if(e) {
        if(e == boost::asio::error::eof)
            XDEBUG_WITH_ID << "The remote peer closed the connection.";
        else {
            XWARN_WITH_ID << "Failed to read body from remote server, message: " << e.message();
            callback_(e);
            return;
        }
    }

    std::size_t read = remote_buffer_.size();
    boost::asio::streambuf::mutable_buffers_type buf = response_->body().prepare(read);
    std::size_t copied = boost::asio::buffer_copy(buf, remote_buffer_.data());

    XTRACE_WITH_ID << "Body from remote server, size: " << read
           << ", body copied from raw stream to response, copied: " << copied
           << ", current body size: " << response_->body().size() + copied
           << ", desired body size: " << response_->body_length();

    if(copied < read) {
        // copied should always equal to read, so here just output an error log
        XERROR_WITH_ID << "The copied size(" << copied << ") is less than read size(" << read
               << "), but this should never happen.";
    }

    response_->body().commit(copied);
    remote_buffer_.consume(copied);

    if(response_->body().size() < response_->body_length()) { // there is more content
        if(is_ssl_) {
            boost::asio::async_read(*ssl_socket_, remote_buffer_,
                                    boost::asio::transfer_at_least(1),
                                    strand_.wrap(boost::bind(&HttpClient::OnRemoteBodyReceived,
                                                             this,
                                                             boost::asio::placeholders::error)));
        } else {
            boost::asio::async_read(*socket_, remote_buffer_,
                                    boost::asio::transfer_at_least(1),
                                    strand_.wrap(boost::bind(&HttpClient::OnRemoteBodyReceived,
                                                             this,
                                                             boost::asio::placeholders::error)));
        }
    } else {
        // TODO do something here
        state_ = kAvailable;
        callback_(e);
        //boost::system::error_code ec;
        //remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    }
}

bool HttpClient::VerifyCertificate(bool pre_verified, boost::asio::ssl::verify_context& ctx) {
    // TODO enhance this function
    char subject_name[256];
    X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    XDEBUG_WITH_ID << "Verify remote certificate, subject name: " << subject_name
           << ", pre_verified value: " << pre_verified;

    return true;
}
