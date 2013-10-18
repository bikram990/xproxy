#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "http_client.h"
#include "log.h"

HttpClient::HttpClient(boost::asio::io_service& service,
                       HttpRequest& request,
                       boost::asio::ssl::context *context)
    : service_(service), resolver_(service), is_ssl_(context ? true : false),
      request_(request) {
    TRACE_THIS_PTR;
    if(context)
        ssl_socket_.reset(new ssl_socket(service_, *context));
    else
        socket_.reset(new socket(service_));
}

HttpClient::~HttpClient() {
    TRACE_THIS_PTR;
}

void HttpClient::AsyncSendRequest(handler_type handler) {
    handler_ = handler;
    ResolveRemote();
}

void HttpClient::ResolveRemote() {
    const std::string host = request_.host();
    short port = request_.port();

    XDEBUG << "Resolving remote address, host: " << host << ", port: " << port;

    boost::asio::ip::tcp::resolver::query query(host, boost::lexical_cast<std::string>(port));
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
        handler_(e, NULL);
        return;
    }
    if(is_ssl_) {
        ssl_socket_->async_handshake(boost::asio::ssl::stream_base::client,
                                     boost::bind(&HttpClient::OnRemoteHandshaken,
                                                 this,
                                                 boost::asio::placeholders::error));
    } else {
        boost::asio::async_write(*socket_, request_.OutboundBuffer(),
                                 boost::bind(&HttpClient::OnRemoteDataSent,
                                             this,
                                             boost::asio::placeholders::error));
    }
}

void HttpClient::OnRemoteHandshaken(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed at handshake step, message: " << e.message();
        handler_(e, NULL);
        return;
    }

    boost::asio::async_write(*ssl_socket_, request_.OutboundBuffer(),
                             boost::bind(&HttpClient::OnRemoteDataSent,
                                         this,
                                         boost::asio::placeholders::error));
}

void HttpClient::OnRemoteDataSent(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to write request to remote server, message: " << e.message();
        handler_(e, NULL);
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
        handler_(e, NULL);
        return;
    }

    // As async_read_until may return more data beyond the delimiter, so we only process the status line
    std::istream response(&remote_buffer_);
    std::getline(response, response_.status_line());
    response_.status_line() += '\n'; // append the missing newline character

    XTRACE << "Status line from remote server: " << response_.status_line();

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
        handler_(e, NULL);
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

        std::string::size_type sep_idx = header.find(": ");
        if(sep_idx == std::string::npos) {
            XWARN << "Invalid header: " << header;
            continue;
        }

        std::string name = header.substr(0, sep_idx);
        std::string value = header.substr(sep_idx + 2, header.length() - 1 - name.length() - 2); // remove the last \r
        response_.AddHeader(name, value);

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
        handler_(e, &response_);
        // TODO do something here
        //boost::system::error_code ec;
        //remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        return;
    }

    response_.body_lenth(body_len);
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
        handler_(e, NULL);
        return;
    }

    std::size_t read = remote_buffer_.size();
    std::size_t copied = boost::asio::buffer_copy(boost::asio::buffer(response_.body()), remote_buffer_.data());

    XTRACE << "Chunk from remote server, read size: " << read;
    XTRACE << "Body copied from raw stream to response, copied: " << copied;

    if(copied < read) {
        // TODO here we should handle the condition that the buffer size is
        // smaller than the raw stream size
    }

    remote_buffer_.consume(read);

    bool finished = false;
    if(response_.body()[copied - 4] == '\r' && response_.body()[copied - 3] == '\n'
                    && response_.body()[copied - 2] == '\r' && response_.body()[copied - 1] == '\n')
            finished = true;

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
        handler_(e, &response_);
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
            handler_(e, NULL);
            return;
        }
    }

    std::size_t read = remote_buffer_.size();
    std::size_t copied = boost::asio::buffer_copy(boost::asio::buffer(response_.body()), remote_buffer_.data());

    XDEBUG << "Body from remote server, size: " << read
           << ", content:\n" << boost::asio::buffer_cast<const char *>(remote_buffer_.data());
    XDEBUG << "Body copied from raw stream to response, copied: " << copied
           << ", response body size: " << response_.body().size();

    if(copied < read) {
        // TODO the response's body buffer is less than read content, try write again
    }

    remote_buffer_.consume(read); // the read bytes are consumed

    if(read < response_.body_length()) { // there is more content
        response_.body_lenth(response_.body_length() - read);
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
        handler_(e, &response_);
        //boost::system::error_code ec;
        //remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    }
}
