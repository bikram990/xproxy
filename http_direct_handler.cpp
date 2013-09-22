#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "log.h"
#include "http_direct_handler.h"
#include "http_proxy_session.h"

HttpDirectHandler::HttpDirectHandler(HttpProxySession& session,
                                     boost::asio::io_service& service,
                                     boost::asio::ip::tcp::socket& local_socket,
                                     boost::asio::ip::tcp::socket& remote_socket)
    : session_(session), local_socket_(local_socket),
      remote_socket_(remote_socket), resolver_(service) {
        TRACE_THIS_PTR;
}

HttpDirectHandler::~HttpDirectHandler() {
    TRACE_THIS_PTR;
}

void HttpDirectHandler::HandleRequest(char *data, std::size_t size) {
    XTRACE << "Raw request from " << local_socket_.remote_endpoint().address()
           << ":" << local_socket_.remote_endpoint().port()
           << ":\n" << data;
    ResultType result = request_.BuildFromRaw(data, size);
    if(result == HttpRequest::kComplete) {
        ResolveRemote();
    } else if(result == HttpRequest::kNotComplete) {
        XWARN << "Fuck, the request parsing seems failed, or maybe a https connection...";
        session_.Terminate();
    } else {
        XWARN << "Bad request: " << local_socket_.remote_endpoint().address()
              << ":" << local_socket_.remote_endpoint().port();
        // TODO implement this
        session_.Terminate();
    }
}

void HttpDirectHandler::ResolveRemote() {
    const std::string& host = request_.host();
    short port = request_.port();

    XDEBUG << "Resolving remote address, host: " << host << ", port: " << port;

    boost::asio::ip::tcp::resolver::query query(host, boost::lexical_cast<std::string>(port));
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query);

    XDEBUG << "Connecting to remote address: " << endpoint_iterator->endpoint().address();

    boost::asio::async_connect(remote_socket_, endpoint_iterator,
                               boost::bind(&HttpDirectHandler::HandleConnect,
                                           this,
                                           boost::asio::placeholders::error));
}

void HttpDirectHandler::HandleConnect(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to connect to remote server, message: " << e.message();
        session_.Stop();
        return;
    }
    boost::asio::async_write(remote_socket_, request_.OutboundBuffer(),
                             boost::bind(&HttpDirectHandler::HandleRemoteWrite,
                                         this,
                                         boost::asio::placeholders::error));
}

void HttpDirectHandler::HandleRemoteWrite(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to write request to remote server, message: " << e.message();
        session_.Stop();
        return;
    }
    boost::asio::async_read_until(remote_socket_, remote_buffer_, "\r\n",
                                  boost::bind(&HttpDirectHandler::HandleRemoteReadStatusLine,
                                              this,
                                              boost::asio::placeholders::error));
}

void HttpDirectHandler::HandleRemoteReadStatusLine(const boost::system::error_code& e) {
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

    //boost::asio::buffer_copy(boost::asio::buffer(local_buffer_), boost::asio::buffer(status_line));

    boost::asio::async_write(local_socket_, boost::asio::buffer(response_.status_line()),
                             boost::bind(&HttpDirectHandler::HandleLocalWrite,
                                         this,
                                         boost::asio::placeholders::error, false));

    boost::asio::async_read_until(remote_socket_, remote_buffer_, "\r\n\r\n",
                                  boost::bind(&HttpDirectHandler::HandleRemoteReadHeaders,
                                              this,
                                              boost::asio::placeholders::error));
}

void HttpDirectHandler::HandleRemoteReadHeaders(const boost::system::error_code& e) {
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

        if(name != "Content-Length")
            continue;
        body_len = boost::lexical_cast<std::size_t>(value);
    }

    boost::asio::async_write(local_socket_, boost::asio::buffer(response_.headers()),
                             boost::bind(&HttpDirectHandler::HandleLocalWrite,
                                         this,
                                         boost::asio::placeholders::error,
                                         body_len <= 0));

    if(body_len <= 0) {
        XDEBUG << "This response seems have no body.";
        boost::system::error_code ec;
        remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        return;
    }

    response_.SetBodyLength(body_len);
    boost::asio::async_read(remote_socket_, remote_buffer_, boost::asio::transfer_at_least(1),
                            boost::bind(&HttpDirectHandler::HandleRemoteReadBody,
                                        this,
                                        boost::asio::placeholders::error));
}

void HttpDirectHandler::HandleRemoteReadBody(const boost::system::error_code& e) {
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
                             boost::bind(&HttpDirectHandler::HandleLocalWrite,
                                         this,
                                         boost::asio::placeholders::error, read >= response_.GetBodyLength()));
    if(copied < read) {
        // TODO the response's body buffer is less than read content, try write again
    }

    remote_buffer_.consume(read); // the read bytes are consumed

    if(read < response_.GetBodyLength()) { // there is more content
        response_.SetBodyLength(response_.GetBodyLength() - read);
        boost::asio::async_read(remote_socket_, remote_buffer_, boost::asio::transfer_at_least(1/*body_len*/),
                                boost::bind(&HttpDirectHandler::HandleRemoteReadBody,
                                            this,
                                            boost::asio::placeholders::error));
    } else {
        boost::system::error_code ec;
        remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    }
}

void HttpDirectHandler::HandleLocalWrite(const boost::system::error_code& e,
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
