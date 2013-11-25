#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "http_client.h"
#include "http_request.h"
#include "http_response.h"
#include "log.h"
#include "resource_manager.h"

boost::atomic<std::size_t> HttpClient::counter_(0);

HttpClient::HttpClient(boost::asio::io_service& service)
    : id_(counter_), state_(kAvailable),
      service_(service), strand_(service), resolver_(service),
      timeout_(service),
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
        state_ = kAvailable;
        return;
    }

    callback_ = callback;

    if(!request || !response) {
        XERROR_WITH_ID << "Invalid request or response.";
        InvokeCallback(boost::asio::error::invalid_argument);
        return;
    }
    if(persistent_) {
        if(host_ != request->host() || port_ != request->port()) {
            XERROR_WITH_ID << "Destination mismatch, required destination: ["
                           << request->host() << ":" << request->port()
                           << "], available persistent destination: ["
                           << host_ << ":" << port_ << "].";
            InvokeCallback(boost::asio::error::address_in_use);
            return;
        }
    }

    request_ = request;
    response_ = response;
    host_ = request->host();
    port_ = request->port();
    state_ = kInUse;

    if(!persistent_) {
        socket_.reset(Socket::Create(service_));
        if(mode == HttpProxySession::HTTPS) {
            is_ssl_ = true;
            socket_->SwitchProtocol<ResourceManager::CertManager::CAPtr,
                    ResourceManager::CertManager::DHParametersPtr>(kHttps);
        }
        ResolveRemote();
    } else {
        if((is_ssl_ && mode != HttpProxySession::HTTPS) ||
           (!is_ssl_ && mode == HttpProxySession::HTTPS)) {
            XERROR_WITH_ID << "Mismatch protocol type.";
            InvokeCallback(boost::asio::error::operation_not_supported);
            return;
        }
        /*
         * No need to judge by is_open() method, this is a NC method!!!
         * it will crash when socket is newly created, it will return true when
         * socket is already closed, is there any useful place of this method??
         * No, I definitely think NO.
         */
        // if(!socket_->is_open()) {
        //     XERROR_WITH_ID << "Socket is already closed.";
        //     InvokeCallback(boost::asio::error::not_connected);
        //     return;
        // }

        XDEBUG_WITH_ID << "Dump request before sending(size: " << request_->OutboundBuffer().size() << "):\n"
                       << "--------------------------------------------\n"
                       << boost::asio::buffer_cast<const char *>(request_->OutboundBuffer().data())
                       << "\n--------------------------------------------";

        timeout_.cancel(); // cancel the timer which is not triggered yet

        boost::asio::async_write(*socket_, boost::asio::buffer(request_->OutboundBuffer().data(), request_->OutboundBuffer().size()),
                                 strand_.wrap(boost::bind(&HttpClient::OnRemoteDataSent,
                                                          this,
                                                          boost::asio::placeholders::error)));
    }

    persistent_ = false; // reset persistent_ to false
}

void HttpClient::ResolveRemote() {
    XDEBUG_WITH_ID << "Resolving remote address, host: " << host_ << ", port: " << port_;

    try {
        boost::asio::ip::tcp::resolver::query query(host_, boost::lexical_cast<std::string>(port_));
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query);

        XDEBUG_WITH_ID << "Connecting to remote address: " << endpoint_iterator->endpoint().address();

        socket_->async_connect(endpoint_iterator, strand_.wrap(boost::bind(&HttpClient::OnRemoteConnected,
                                                                           this,
                                                                           boost::asio::placeholders::error)));
    } catch(const boost::system::system_error& e) {
        XERROR_WITH_ID << "Failed to resolve [" << host_ << ":" << port_ << "], error: " << e.what();
        InvokeCallback(e.code());
    }
}

void HttpClient::OnRemoteConnected(const boost::system::error_code& e) {
    if(e) {
        XERROR_WITH_ID << "Failed to connect to remote server, message: " << e.message();
        InvokeCallback(e);
        return;
    }

    XDEBUG_WITH_ID << "Dump request before sending(size: " << request_->OutboundBuffer().size() << "):\n"
                   << "--------------------------------------------\n"
                   << boost::asio::buffer_cast<const char *>(request_->OutboundBuffer().data())
                   << "\n--------------------------------------------";

    boost::asio::async_write(*socket_, boost::asio::buffer(request_->OutboundBuffer().data(), request_->OutboundBuffer().size()),
                             strand_.wrap(boost::bind(&HttpClient::OnRemoteDataSent,
                                                      this,
                                                      boost::asio::placeholders::error)));
}

void HttpClient::OnRemoteDataSent(const boost::system::error_code& e) {
    if(e) {
        XERROR_WITH_ID << "Failed to write request to remote server, message: " << e.message();
        InvokeCallback(e);
        return;
    }

    boost::asio::async_read_until(*socket_, response_->InboundBuffer(), "\r\n",
                                  strand_.wrap(boost::bind(&HttpClient::OnRemoteStatusLineReceived,
                                                           this,
                                                           boost::asio::placeholders::error)));
}

void HttpClient::OnRemoteStatusLineReceived(const boost::system::error_code& e) {
    if(e) {
        if(e == boost::asio::error::eof || SSL_SHORT_READ(e)) {
            XWARN_WITH_ID << "Remote server has already closed the persistent connection, reconnect...";
            persistent_ = false;
            socket_.reset(Socket::Create(service_));
            if(is_ssl_) {
                socket_->SwitchProtocol<ResourceManager::CertManager::CAPtr,
                        ResourceManager::CertManager::DHParametersPtr>(kHttps);
            }
            ResolveRemote();
            return;
        }
        XERROR_WITH_ID << "Failed to read status line from remote server, message: " << e.message();
        InvokeCallback(e);
        return;
    }

    // As async_read_until may return more data beyond the delimiter, so we only process the status line
    response_->ConsumeInitialLine();

    XTRACE_WITH_ID << "Status code and message from remote server:\n"
                   << "--------------------------------------------\n"
                   << response_->StatusCode() << ", " << response_->StatusMessage()
                   << "\n--------------------------------------------";

    boost::asio::async_read_until(*socket_, response_->InboundBuffer(), "\r\n\r\n",
                                  strand_.wrap(boost::bind(&HttpClient::OnRemoteHeadersReceived,
                                                           this,
                                                           boost::asio::placeholders::error)));
}

void HttpClient::OnRemoteHeadersReceived(const boost::system::error_code& e) {
    if(e) {
        XERROR_WITH_ID << "Failed to read response header from remote server, message: " << e.message();
        InvokeCallback(e);
        return;
    }

    XTRACE_WITH_ID << "Headers from remote server:\n"
                   << "--------------------------------------------\n"
                   << boost::asio::buffer_cast<const char *>(response_->InboundBuffer().data())
                   << "\n--------------------------------------------";

    response_->ConsumeHeaders();

    bool chunked = false;
    std::string transfer_encoding;
    if(response_->FindHeader("Transfer-Encoding", transfer_encoding)) {
        XDEBUG_WITH_ID << "Transfer-Encoding header is found, value: " << transfer_encoding;
        if(transfer_encoding == "chunked")
            chunked = true;
    }

    std::size_t body = 0;
    std::string content_length;
    if(response_->FindHeader("Content-Length", content_length)) {
        if(chunked)
            XWARN_WITH_ID << "Both Transfer-Encoding and Content-Length headers are found, but this would never happen.";
        body = boost::lexical_cast<std::size_t>(content_length);
    }

    std::string connection;
    if(response_->FindHeader("Connection", connection)) {
        std::transform(connection.begin(), connection.end(), connection.begin(), std::ptr_fun<int, int>(std::tolower));
        if(connection == "keep-alive") {
            persistent_ = true;
            XDEBUG_WITH_ID << "This is a persistent connection.";
        }
    }

    if(chunked) {
        if(response_->InboundBuffer().size() > 0)
            OnRemoteChunksReceived(boost::system::error_code());
        else
            boost::asio::async_read(*socket_, response_->InboundBuffer(),
                                    boost::asio::transfer_at_least(1),
                                    strand_.wrap(boost::bind(&HttpClient::OnRemoteChunksReceived,
                                                             this,
                                                             boost::asio::placeholders::error)));
        return;
    }

    if(body > 0) {
        response_->SetBodyLength(body); // we have to record the body length here
        if(response_->InboundBuffer().size() > 0)
            OnRemoteBodyReceived(boost::system::error_code());
        else
            boost::asio::async_read(*socket_, response_->InboundBuffer(),
                                    boost::asio::transfer_at_least(1),
                                    strand_.wrap(boost::bind(&HttpClient::OnRemoteBodyReceived,
                                                             this,
                                                             boost::asio::placeholders::error)));
        return;
    }

    XDEBUG_WITH_ID << "This response seems have no body.";
    // TODO do something here
    if(!persistent_)
        socket_->close();
    else {
        timeout_.expires_from_now(boost::posix_time::seconds(kDefaultTimeout));
        timeout_.async_wait(boost::bind(&HttpClient::OnTimeout, this, boost::asio::placeholders::error));
    }
    InvokeCallback(e);
}

void HttpClient::OnRemoteChunksReceived(const boost::system::error_code& e) {
    if(e) {
        XERROR_WITH_ID << "Failed to read chunk from remote server, message: " << e.message();
        InvokeCallback(e);
        return;
    }

    response_->ConsumeBody();

    bool finished = false;
    const char *body = boost::asio::buffer_cast<const char *>(response_->body().data());
    std::size_t size = response_->body().size();
    if(body[size - 4] == '\r'
       && body[size -3] == '\n'
       && body[size - 2] == '\r'
       && body[size - 1] == '\n') {
        XTRACE_WITH_ID << "The end of all chunks has been reached.";
        response_->SetBodyLength(size);
        finished = true;
    }

    if(!finished) {
        boost::asio::async_read(*socket_, response_->InboundBuffer(),
                                boost::asio::transfer_at_least(1),
                                strand_.wrap(boost::bind(&HttpClient::OnRemoteChunksReceived,
                                                         this,
                                                         boost::asio::placeholders::error)));
    } else {
        // TODO do something here
        if(!persistent_)
            socket_->close();
        else {
            timeout_.expires_from_now(boost::posix_time::seconds(kDefaultTimeout));
            timeout_.async_wait(boost::bind(&HttpClient::OnTimeout, this, boost::asio::placeholders::error));
        }
        InvokeCallback(e);
    }
}

void HttpClient::OnRemoteBodyReceived(const boost::system::error_code& e) {
    if(e) {
        if(e == boost::asio::error::eof || SSL_SHORT_READ(e)) {
            XDEBUG_WITH_ID << "The remote peer closed the connection, socket: " << socket_->to_string() << ".";
            persistent_ = false;
        } else {
            XERROR_WITH_ID << "Failed to read body from remote server, message: " << e.message();
            InvokeCallback(e);
            return;
        }
    }

    response_->ConsumeBody(false);

    if(response_->body().size() < response_->BodyLength()) { // there is more content
        boost::asio::async_read(*socket_, response_->InboundBuffer(),
                                boost::asio::transfer_at_least(1),
                                strand_.wrap(boost::bind(&HttpClient::OnRemoteBodyReceived,
                                                         this,
                                                         boost::asio::placeholders::error)));
    } else {
        // TODO do something here
        if(!persistent_)
            socket_->close();
        else {
            timeout_.expires_from_now(boost::posix_time::seconds(kDefaultTimeout));
            timeout_.async_wait(boost::bind(&HttpClient::OnTimeout, this, boost::asio::placeholders::error));
        }
        InvokeCallback(e);
    }
}

void HttpClient::OnTimeout(const boost::system::error_code& e) {
    if(e == boost::asio::error::operation_aborted)
        XDEBUG_WITH_ID << "The timeout timer is cancelled.";
    else if(e)
        XERROR_WITH_ID << "Error occurred with the timer, message: " << e.message();
    else if(timeout_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
        XDEBUG_WITH_ID << "Socket timeout, close it.";
        persistent_ = false;
        socket_->close();
    }
}
