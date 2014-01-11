#include <boost/lexical_cast.hpp>
#include "http_request_initial.h"
#include "http_response_initial.h"
#include "session.h"

boost::atomic<std::size_t> Session::counter_(0);

void Session::start() {
    if(client_in_.size() > 0) {
        XWARN_WITH_ID << "Client in buffer is not empty, but this should never happen.";
        client_in_.consume(client_in_.size());
    }

    client_in_.prepare(kDefaultClientInBufferSize);
    boost::asio::async_read(*client_socket_, client_in_,
                            boost::asio::transfer_at_least(1),
                            boost::bind(&this_type::OnClientDataReceived,
                                        shared_from_this(),
                                        boost::asio::placeholders::error));
}

void Session::stop() {
    client_socket_->close();
    server_socket_->close();
}

void Session::AsyncReadFromClient() {
    if(client_in_.size() > 0) {
        XDEBUG_WITH_ID << "There is still data in client in buffer, skip reading from socket.";
        service_.post(boost::bind(&this_type::OnClientDataReceived,
                                  shared_from_this(),
                                  boost::system::error_code()));
        return;
    }

    client_in_.prepare(kDefaultClientInBufferSize);
    boost::asio::async_read(*client_socket_, client_in_,
                            boost::asio::transfer_at_least(1),
                            boost::bind(&this_type::OnClientDataReceived,
                                        shared_from_this(),
                                        boost::asio::placeholders::error));
}

void Session::AsyncWriteSSLReplyToClient() {
    if(client_out_.size() > 0)
        client_out_.consume(client_out_.size());

    for(std::size_t i = 0; i < response_->size(); ++i) {
        SharedBuffer buffer = response_->RetrieveObject(i)->ByteContent();
        std::size_t copied = boost::asio::buffer_copy(client_out_.prepare(buffer->size()),
                                                      boost::asio::buffer(buffer->data(),
                                                                          buffer->size()));
        assert(copied == buffer->size());
        client_out_.commit(copied);
    }

    client_socket_->async_write_some(boost::asio::buffer(client_out_.data(),
                                                         client_out_.size()),
                                     boost::bind(&this_type::OnClientSSLReplySent,
                                                 shared_from_this(),
                                                 boost::asio::placeholders::error));
}

void Session::AsyncConnectToServer() {
    // we switch to https mode before we connect to server
    if(https_) {
        server_socket_->SwitchProtocol<ResourceManager::CertManager::CAPtr,
                ResourceManager::CertManager::DHParametersPtr>(kHttps);
    }

    try {
        boost::asio::ip::tcp::resolver::query query(host_, boost::lexical_cast<std::string>(port_));
        auto endpoint_iterator = resolver_.resolve(query);

        XDEBUG_WITH_ID << "Connecting to remote, host: " << host_
                       << ", port: " << port_ << ", resolved address: "
                       << endpoint_iterator->endpoint().address();

        server_socket_->async_connect(endpoint_iterator,
                                      boost::bind(&this_type::OnServerConnected,
                                                  shared_from_this(),
                                                  boost::asio::placeholders::error));
        } catch(const boost::system::system_error& e) {
            XERROR_WITH_ID << "Failed to resolve [" << host_ << ":" << port_ << "], error: " << e.what();
            // TODO add logic here
            manager_.stop(shared_from_this());
        }
}

void Session::AsyncWriteToServer() {
    if(!server_connected_) {
        AsyncConnectToServer();
        return;
    }

    if(server_out_.size() > 0)
        server_out_.consume(server_out_.size());

    for(std::size_t i = 0; i < request_->size(); ++i) {
        SharedBuffer buffer = request_->RetrieveObject(i)->ByteContent();
        std::size_t copied = boost::asio::buffer_copy(server_out_.prepare(buffer->size()),
                                                      boost::asio::buffer(buffer->data(),
                                                                          buffer->size()));
        assert(copied == buffer->size());
        server_out_.commit(copied);
    }

    server_socket_->async_write_some(boost::asio::buffer(server_out_.data(),
                                                         server_out_.size()),
                                     boost::bind(&this_type::OnServerDataSent,
                                                 shared_from_this(),
                                                 boost::asio::placeholders::error));
}

void Session::AsyncReadFromServer() {
    if(server_in_.size() > 0) {
        XDEBUG_WITH_ID << "There is still data in client in buffer, skip reading from socket.";
        service_.post(boost::bind(&this_type::OnServerDataReceived,
                                  shared_from_this(),
                                  boost::system::error_code()));
        return;
    }

    server_in_.prepare(kDefaultServerInBufferSize);
    boost::asio::async_read(*server_socket_, server_in_,
                            boost::asio::transfer_at_least(1),
                            boost::bind(&this_type::OnServerDataReceived,
                                        shared_from_this(),
                                        boost::asio::placeholders::error));
}

void Session::AsyncWriteToClient() {
    if(client_out_.size() > 0)
        client_out_.consume(client_out_.size());

    /// when we write data to client, we only write the latest object,
    /// because the response is written to client one object by one object
    if(response_->size() <= 0) {
        XDEBUG_WITH_ID << "No content in response, a proxied request?";
        return;
    }

    SharedBuffer buffer = response_->RetrieveLatest()->ByteContent();
    std::size_t copied = boost::asio::buffer_copy(client_out_.prepare(buffer->size()),
                                                  boost::asio::buffer(buffer->data(),
                                                                      buffer->size()));
    assert(copied == buffer->size());
    client_out_.commit(copied);

    client_socket_->async_write_some(boost::asio::buffer(client_out_.data(),
                                                         client_out_.size()),
                                     boost::bind(&this_type::OnClientDataSent,
                                                 shared_from_this(),
                                                 boost::asio::placeholders::error));
}

void Session::OnClientDataReceived(const boost::system::error_code& e) {
    if(e) {
        XERROR_WITH_ID << "Error occurred during reading from client socket, message: "
                       << e.message();
        // TODO add logic here
        manager_.stop(shared_from_this());
        return;
    }

    if(client_in_.size() <= 0) {
        XWARN_WITH_ID << "No data in client socket.";
        // TODO add logic here
        manager_.stop(shared_from_this());
        return;
    }

    HttpObject *object = nullptr;
    Decoder::DecodeResult result = request_decoder_->decode(client_in_, &object);

    switch(result) {
    case Decoder::kIncomplete:
        XDEBUG_WITH_ID << "Incomplete buffer, continue reading...";
        AsyncReadFromClient();
        break;
    case Decoder::kFailure:
        XERROR_WITH_ID << "Failed to decode object, return.";
        // TODO add logic here
        manager_.stop(shared_from_this());
        break;
    case Decoder::kComplete:
        XDEBUG_WITH_ID << "One object decoded, continue reading...";
        assert(object != nullptr);
        request_->AppendObject(object);
        AsyncReadFromClient();
        break;
    case Decoder::kFinished: {
        assert(object != nullptr);

        XDEBUG_WITH_ID << "A complete request is decoded.";
        request_->AppendObject(object);

        HttpRequestInitial *initial = reinterpret_cast<HttpRequestInitial *>(request_->RetrieveInitial());
        assert(initial != nullptr);
        if(initial->method() == "CONNECT") {
            /// We do not expect a CONNECT request in a reused connection
            if(reused_) {
                if(https_)
                    XWARN_WITH_ID << "CONNECT request is sent through a persistent https connection ["
                                  << host_ << ":" << port_ << "], will this happen?";
                else
                    XWARN_WITH_ID << "A https CONNECT request is sent through a persistent http connection ["
                                  << host_ << ":" << port_ << "], will this happen?";
                // TODO add logic here
                manager_.stop(shared_from_this());
                return;
            }

            https_ = true;

            /// set host_ and port_
            std::string::size_type sep = initial->uri().find(':');
            if(sep != std::string::npos) {
                port_ = boost::lexical_cast<unsigned short>(initial->uri().substr(sep + 1));
                host_ = initial->uri().substr(0, sep);
            } else {
                port_ = 443;
                host_ = initial->uri();
            }

            /// construct the response
            HttpResponseInitial *ri = new HttpResponseInitial;
            ri->SetMajorVersion(1);
            ri->SetMinorVersion(1);
            ri->SetStatusCode(200);
            ri->StatusMessage().append("Connection Established");
            response_->AppendObject(ri);

            HttpHeaders *headers = new HttpHeaders;
            headers->PushBack(HttpHeader("Proxy-Connection", "keep-alive"));
            headers->PushBack(HttpHeader("Connection", "keep-alive"));
            response_->AppendObject(headers);

            AsyncWriteSSLReplyToClient();
            return;
        }

        /// no matter it is a persistent connection or it is the first request
        /// in the connection, we should set host_ and port_ when it is not a
        /// https request, the difference is, for a reused connection, we need
        /// to verify if the new host and port match the existing ones
        if(!https_) {
            HttpHeaders *headers = request_->RetrieveHeaders();

            assert(headers != nullptr);

            std::string host;
            unsigned short port = 80;
            if(!headers->find("Host", host)) {
                XERROR_WITH_ID << "No host found in header, this should never happen.";
                // TODO add logic here
                manager_.stop(shared_from_this());
                return;
            }

            std::string::size_type sep = host.find(':');
            if(sep != std::string::npos) {
                port = boost::lexical_cast<unsigned short>(host.substr(sep + 1));
                host = host.substr(0, sep);
            }

            if(reused_) {
                if(host != host_ || port != port_) {
                    XWARN_WITH_ID << "Host or port ["
                                  << host << ":" << port << "]"
                                  << " mismatch in a persistent connection ["
                                  << host_ << ":" << port_ << "], will this happen?";
                    // TODO add logic here
                    manager_.stop(shared_from_this());
                    return;
                }
            } else {
                host_ = host;
                port_ = port;
            }
        }

        /// canonicalize the URI
        if(initial->uri()[0] != '/') { // the URI is not canonicalized
            std::string http("http://");
            std::string::size_type end = std::string::npos;

            if(initial->uri().compare(0, http.length(), http) != 0) {
                end = initial->uri().find('/');
            } else {
                end = initial->uri().find('/', http.length());
            }

            if(end != std::string::npos) {
                initial->uri().erase(0, end);
            } else {
                XDEBUG_WITH_ID << "No host end / found, consider as root: " << initial->uri();
                initial->uri() = '/';
            }
        }

        HttpContainer *response = chain_->FilterRequest(request_);
        if(response) {
            XDEBUG_WITH_ID << "Filters don't want to send this request.";
            delete response_;
            response_ = response;
            AsyncWriteToClient();
        } else {
            AsyncWriteToServer();
        }
        break;
    }
    default:
        XERROR_WITH_ID << "Invalid result: " << static_cast<int>(result);
        break;
    }
}

void Session::OnClientSSLReplySent(const boost::system::error_code& e) {
    if(e) {
        XERROR_WITH_ID << "Error occurred during writing SSL OK reply to socket, message: "
                       << e.message();
        // TODO add logic here
        manager_.stop(shared_from_this());
        return;
    }

    InitClientSSLContext();

    // because we have decoded "CONNECT..." request, so we reset it here
    // to decode new request
    request_decoder_->reset();

    // also reset the container, to clear previously decoded http object
    request_->reset();

    AsyncReadFromClient();
}

void Session::OnServerConnected(const boost::system::error_code& e) {
    if(e) {
        XERROR_WITH_ID << "Error occurred during connecting to remote peer, message: "
                       << e.message();
        // TODO add logic here
        manager_.stop(shared_from_this());
        return;
    }

    XDEBUG_WITH_ID << "Remote peer connected: " << host_ << ":" << port_;
    server_connected_ = true;
    AsyncWriteToServer();
}

void Session::OnServerDataSent(const boost::system::error_code& e) {
    if(e) {
        XERROR_WITH_ID << "Error occurred during writing to remote peer, message: "
                       << e.message();
        // TODO add logic here
        manager_.stop(shared_from_this());
        return;
    }

    XDEBUG << "Data has been written to remote peer, now start reading...";
    AsyncReadFromServer();
}

void Session::OnServerDataReceived(const boost::system::error_code& e) {
    if(e) {
        XERROR_WITH_ID << "Error occurred during reading from server socket, message: "
                       << e.message();
        // TODO add logic here
        manager_.stop(shared_from_this());
        return;
    }

    if(server_in_.size() <= 0) {
        XWARN_WITH_ID << "No data in server socket.";
        // TODO add logic here
        manager_.stop(shared_from_this());
        return;
    }

    HttpObject *object = nullptr;
    Decoder::DecodeResult result = response_decoder_->decode(server_in_, &object);

    switch(result) {
    case Decoder::kIncomplete:
        XDEBUG_WITH_ID << "Incomplete buffer, continue reading...";
        AsyncReadFromServer();
        break;
    case Decoder::kFailure:
        XERROR_WITH_ID << "Failed to decode object, return.";
        // TODO add logic here
        manager_.stop(shared_from_this());
        break;
    case Decoder::kComplete:
        assert(object != nullptr);
        response_->AppendObject(object);
        chain_->FilterResponse(response_);
        AsyncWriteToClient();
        AsyncReadFromServer();
        break;
    case Decoder::kFinished:
        assert(object != nullptr);
        finished_ = true;
        response_->AppendObject(object);
        chain_->FilterResponse(response_);
        AsyncWriteToClient();

        server_timer_.expires_from_now(boost::posix_time::seconds(kDefaultServerTimeoutValue));
        server_timer_.async_wait(boost::bind(&this_type::OnServerTimeout,
                                             shared_from_this(),
                                             boost::asio::placeholders::error));
        break;
    default:
        XERROR_WITH_ID << "Invalid result: " << static_cast<int>(result);
        break;
    }
}

void Session::OnClientDataSent(const boost::system::error_code& e) {
    if(e) {
        XERROR_WITH_ID << "Error occurred during writing to client socket, message: "
                       << e.message();
        // TODO add logic here
        manager_.stop(shared_from_this());
        return;
    }

    if(!finished_) {
        XDEBUG_WITH_ID << "There is still data needed by client socket, do nothing.";
        return;
    }

    /// we always treat client connection as persistent connection
    reset();
    client_timer_.expires_from_now(boost::posix_time::seconds(kDefaultClientTimeoutValue));
    client_timer_.async_wait(boost::bind(&this_type::OnClientTimeout,
                                         shared_from_this(),
                                         boost::asio::placeholders::error));

    reused_ = true;

    start();
}

void Session::OnServerTimeout(const boost::system::error_code& e) {
    if(e == boost::asio::error::operation_aborted)
        XDEBUG_WITH_ID << "The server timeout timer is cancelled.";
    else if(e)
        XERROR_WITH_ID << "Error occurred with server timer, message: "
                       << e.message();
    else if(server_timer_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
        XDEBUG_WITH_ID << "Server socket timed out, close it.";
        server_connected_ = false;
        server_socket_->close();
        delete server_socket_;
        server_socket_ = Socket::Create(service_);
    }
}

void Session::OnClientTimeout(const boost::system::error_code& e) {
    if(e == boost::asio::error::operation_aborted)
        XDEBUG_WITH_ID << "The client timeout timer is cancelled.";
    else if(e)
        XERROR_WITH_ID << "Error occurred with client timer, message: "
                       << e.message();
    else if(client_timer_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
        XDEBUG_WITH_ID << "Client socket timed out, close it.";
        // TODO add logic here
        manager_.stop(shared_from_this());
    }
}
