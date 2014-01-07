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
    if(https) {
        server_socket_->SwitchProtocol<ResourceManager::CertManager::CAPtr,
                ResourceManager::CertManager::DHParametersPtr>(kHttps);
    }

    try {
        boost::asio::ip::tcp::resolver::query query(host_, boost::lexical_cast<std::string>(port_));
        auto endpoint_iterator = resolver_.resolve(query);

        XDEBUG_WITH_ID << "Connecting to remote address: "
                       << endpoint_iterator->endpoint().address();

        server_socket_->async_connect(endpoint_iterator,
                                      boost::bind(&this_type::OnServerConnected,
                                                  shared_from_this(),
                                                  boost::asio::placeholders::error));
        } catch(const boost::system::system_error& e) {
            XERROR_WITH_ID << "Failed to resolve [" << host_ << ":" << port_ << "], error: " << e.what();
            // TODO add logic here
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
                                     boost::bind(&this_type::OnClientDataSent,
                                                 shared_from_this(),
                                                 boost::asio::placeholders::error));
}

void Session::OnClientDataReceived(const boost::system::error_code& e) {
    if(e) {
        XERROR_WITH_ID << "Error occurred during reading from client socket, message: "
                       << e.message();
        // TODO add logic here
        return;
    }

    if(client_in_.size() <= 0) {
        XWARN_WITH_ID << "No data in client socket.";
        // TODO add logic here
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
        break;
    case Decoder::kComplete:
        XDEBUG_WITH_ID << "One object decoded, continue reading...";
        assert(object != nullptr);
        request_->AppendObject(object);
        AsyncReadFromClient();
        break;
    case Decoder::kFinished:
        assert(object != nullptr);
        request_->AppendObject(object);

        HttpRequestInitial *initial = reinterpret_cast<HttpRequestInitial *>(request_->RetrieveInitial());
        assert(initial != nullptr);
        if(initial->method() == "CONNECT") {
            https_ = true;

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
        return;
    }

    if(server_in_.size() <= 0) {
        XWARN_WITH_ID << "No data in server socket.";
        // TODO add logic here
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
        break;
    case Decoder::kComplete:
    case Decoder::kFinished:
        // TODO fix me: enhance the logic here
        assert(object != nullptr);
        response_->AppendObject(object);
        chain_->FilterResponse(response_);
        AsyncWriteToClient();
        break;
    default:
        XERROR_WITH_ID << "Invalid result: " << static_cast<int>(result);
        break;
    }
}

void Session::OnClientDataSent(const boost::system::error_code& e) {
}

void Session::OnServerTimeout(const boost::system::error_code& e) {
}

void Session::OnClientTimeout(const boost::system::error_code& e) {
}
