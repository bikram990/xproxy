#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "log.h"
#include "http_proxy_session.h"
#include "http_proxy_session_manager.h"

HttpProxySession::HttpProxySession(boost::asio::io_service& service,
                                   HttpProxySessionManager& manager)
    : local_socket_(service), remote_socket_(service),
      resolver_(service), manager_(manager), request_() {
    TRACE_THIS_PTR;
}

HttpProxySession::~HttpProxySession() {
    TRACE_THIS_PTR;
}

boost::asio::ip::tcp::socket& HttpProxySession::LocalSocket() {
    return local_socket_;
}

boost::asio::ip::tcp::socket& HttpProxySession::RemoteSocket() {
    return remote_socket_;
}

void HttpProxySession::Start() {
    local_socket_.async_read_some(
                boost::asio::buffer(local_buffer_),
                boost::bind(&HttpProxySession::HandleLocalRead,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
}

void HttpProxySession::Stop() {
    local_socket_.close();
    remote_socket_.close();

    // TODO maybe we remote this session from session manager here?
}

void HttpProxySession::HandleLocalRead(const boost::system::error_code &e,
                                       std::size_t size) {
    // TODO dump the raw est to log according a configuration item
    XTRACE << "Raw request: " << std::endl << local_buffer_.data();
    ResultType result = request_.BuildFromRaw(local_buffer_.data(), size);
    if(result == HttpProxyRequest::kComplete) {
        // boost::asio::streambuf response;
        // if(!manager_.dispatcher().DispatchRequest(request_.host(),
        //                                           request_.port(),
        //                                           &request_,
        //                                           response)) {
        //     xwarn << "dispatcher returns false.";
        // }
        // boost::asio::async_write(local_socket_, response,
        //                          boost::bind(&HttpProxySession::HandleLocalWrite,
        //                                      shared_from_this(),
        //                                      boost::asio::placeholders::error));
        ResolveRemote();
    } else if(result == HttpProxyRequest::kNotComplete) {
        //        local_socket_.async_read_some(
        //                    boost::asio::buffer(buffer_),
        //                    boost::bind(&HttpProxySession::HandleRead,
        //                                shared_from_this(),
        //                                boost::asio::placeholders::error,
        //                                boost::asio::placeholders::bytes_transferred));
        XWARN << "Fuck, the request parsing seems failed, or maybe a https connection...";
    } else { // build error
        XWARN << "Bad request: " << local_socket_.remote_endpoint().address()
              << ":" << local_socket_.remote_endpoint().port();
        // TODO implement this
    }
}

void HttpProxySession::HandleLocalWrite(const boost::system::error_code& e,
                                        bool finished) {
    if(e) {
        XWARN << "Failed to write response to local socket, message: " << e.message();
        Stop();
        return;
    }

    XDEBUG << "Content written to local socket: \n" << boost::asio::buffer_cast<const char *>(boost::asio::buffer(local_buffer_));

    if(!finished)
        return;

    if(!e) {
        boost::system::error_code ec;
        local_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        //remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    }

    if(e != boost::asio::error::operation_aborted) {
        manager_.Stop(shared_from_this());
    }
}

void HttpProxySession::ResolveRemote() {
    const std::string& host = request_.host();
    short port = request_.port();

    XDEBUG << "Resolving remote address, host: " << host << ", port: " << port;

    boost::asio::ip::tcp::resolver::query query(host, boost::lexical_cast<std::string>(port));
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query);

    XDEBUG << "Connecting to remote address: " << endpoint_iterator->endpoint().address().to_string();

    boost::asio::async_connect(remote_socket_, endpoint_iterator,
                               boost::bind(&HttpProxySession::HandleConnect,
                                           shared_from_this(),
                                           boost::asio::placeholders::error));
}

void HttpProxySession::HandleConnect(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to connect to remote server, message: " << e.message();
        Stop();
        return;
    }
    boost::asio::async_write(remote_socket_, request_.OutboundBuffer(),
                             boost::bind(&HttpProxySession::HandleRemoteWrite,
                                         shared_from_this(),
                                         boost::asio::placeholders::error));
}

void HttpProxySession::HandleRemoteWrite(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to write request to remote server, message: " << e.message();
        Stop();
        return;
    }
    boost::asio::async_read_until(remote_socket_, remote_buffer_, "\r\n",
                                  boost::bind(&HttpProxySession::HandleRemoteReadStatusLine,
                                              shared_from_this(),
                                              boost::asio::placeholders::error));
}

void HttpProxySession::HandleRemoteReadStatusLine(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to read status line from remote server, message: " << e.message();
        Stop();
        return;
    }

    // As async_read_until may return more data beyond the delimiter, so we only process the status line
    std::istream response(&remote_buffer_);
    std::string status_line;
    std::getline(response, status_line);

    XDEBUG << "Status line from remote server: " << status_line;

    boost::asio::buffer_copy(boost::asio::buffer(local_buffer_), boost::asio::buffer(status_line));

    boost::asio::async_write(local_socket_, /*remote_buffer_*/boost::asio::buffer(local_buffer_),
                             boost::bind(&HttpProxySession::HandleLocalWrite,
                                         shared_from_this(),
                                         boost::asio::placeholders::error, false));

    boost::asio::async_read_until(remote_socket_, remote_buffer_, "\r\n\r\n",
                                  boost::bind(&HttpProxySession::HandleRemoteReadHeaders,
                                              shared_from_this(),
                                              boost::asio::placeholders::error));
}

void HttpProxySession::HandleRemoteReadHeaders(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to read response header from remote server, message: " << e.message();
        Stop();
        return;
    }

    XDEBUG << "Headers from remote server: \n" << boost::asio::buffer_cast<const char *>(remote_buffer_.data());

    boost::asio::buffer_copy(boost::asio::buffer(local_buffer_), remote_buffer_.data());

    std::istream response(&remote_buffer_);
    std::string header;
    std::size_t body_len = 0;
    while(std::getline(response, header)) {
        // TODO here should examine whether the header is a \r character(means a blank line),
        // because async_read_until may return more data then header
        std::string::size_type sep_idx = header.find(": ");
        if(sep_idx == std::string::npos) {
            XWARN << "Invalid header: " << header;
            continue;
        }
        std::string name = header.substr(0, sep_idx);
        if(name != "Content-Length")
            continue;
        body_len = boost::lexical_cast<std::size_t>(header.substr(sep_idx + 2));
    }

    boost::asio::async_write(local_socket_, /*remote_buffer_*/boost::asio::buffer(local_buffer_),
                             boost::bind(&HttpProxySession::HandleLocalWrite,
                                         shared_from_this(),
                                         boost::asio::placeholders::error,
                                         body_len <= 0));

    if(body_len <= 0) {
        XDEBUG << "This response seems have no body.";
        boost::system::error_code ec;
        remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        return;
    }
    boost::asio::async_read(remote_socket_, remote_buffer_, boost::asio::transfer_at_least(body_len),
                            boost::bind(&HttpProxySession::HandleRemoteReadBody,
                                        shared_from_this(),
                                        boost::asio::placeholders::error));
}

void HttpProxySession::HandleRemoteReadBody(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to read body from remote server, message: " << e.message();
        Stop();
        return;
    }

    XDEBUG << "Body from remote server: \n" << boost::asio::buffer_cast<const char *>(remote_buffer_.data());

    // TODO there is no more data from remote socket, so maybe we could use remote_buffer_ directly
    boost::asio::buffer_copy(boost::asio::buffer(local_buffer_), remote_buffer_.data());

    boost::asio::async_write(local_socket_, /*remote_buffer_*/boost::asio::buffer(local_buffer_),
                             boost::bind(&HttpProxySession::HandleLocalWrite,
                                         shared_from_this(),
                                         boost::asio::placeholders::error, true));
    boost::system::error_code ec;
    remote_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
}
