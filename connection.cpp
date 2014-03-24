#include "connection.h"
#include "session.h"

Connection::Connection(std::shared_ptr<Session> session,
                       long timeout,
                       std::size_t buffer_size)
    : session_(session),
      service_(session->service()),
      timer_(session->service()),
      timeout_(timeout),
      socket_(Socket::Create(session->service())),
      connected_(false),
      buffer_size_(buffer_size) {}

Connection::~Connection() {}

void Connection::read() {
    if (!connected_)
        return;

    buffer_in_.prepare(buffer_size_);
    boost::asio::async_read(*socket_, buffer_in_,
                            boost::asio::transfer_at_least(1),
                            std::bind(&Connection::OnRead, this, std::placeholders::_1));
}

void Connection::write() {
    if (!connected_)
        return;

    socket_->async_write_some(boost::asio::buffer(buffer_out_.data(),
                                                  buffer_out_.size()),
                              std::bind(&Connection::OnWritten, this, std::placeholders::_1));
}
