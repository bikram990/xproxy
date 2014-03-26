#include "connection.h"
#include "http_message.h"
#include "log.h"
#include "session.h"

Connection::Connection(std::shared_ptr<Session> session,
                       long timeout,
                       std::size_t buffer_size)
    : session_(session),
      service_(session->service()),
      timer_(session->service()),
      timeout_(timeout),
      timer_triggered_(false),
      socket_(Socket::Create(session->service())),
      connected_(false),
      buffer_size_(buffer_size){}

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

void Connection::ConstructMessage() {
    if (buffer_in_.size() <= 0)
        return;

    if (message_->completed()) {
        //XERROR_WITH_ID << "Message has been completed.";
        return;
    }

    XDEBUG << "Now start to construct the message..., content:\n"
           << std::string(boost::asio::buffer_cast<const char*>(buffer_in_.data()), buffer_in_.size());

    if (!message_->consume(buffer_in_)) {
        //XERROR_WITH_ID << "Parse failed.";
        return;
    }

    std::shared_ptr<Session> s(session_.lock());

    if (s)
        NewDataCallback(s);

    // TODO should we check if buffer_in_ is empty here?

    if (!message_->completed()) {
        read();
        return;
    }

    if (s)
        CompleteCallback(s);
}

void Connection::reset() {
    timer_triggered_ = false;
    message_->reset();

    if (buffer_in_.size() > 0)
        buffer_in_.consume(buffer_in_.size());

    if (buffer_out_.size() > 0)
        buffer_out_.consume(buffer_out_.size());
}
