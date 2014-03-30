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
      timer_running_(false),
      timer_triggered_(false),
      socket_(Socket::Create(session->service())),
      connected_(false),
      buffer_size_(buffer_size){}

void Connection::read() {
    if (!connected_) {
        connect();
        return;
    }

    buffer_in_.prepare(buffer_size_);
    boost::asio::async_read(*socket_, buffer_in_,
                            boost::asio::transfer_at_least(1),
                            std::bind(&Connection::OnRead,
                                      shared_from_this(),
                                      std::placeholders::_1,
                                      std::placeholders::_2));
}

void Connection::write() {
    if (!connected_) {
        connect();
        return;
    }

    XDEBUG_X << "write() called, dump content to be written:\n"
             << std::string(boost::asio::buffer_cast<const char*>(buffer_out_.data()),
                            buffer_out_.size());

    socket_->async_write_some(boost::asio::buffer(buffer_out_.data(),
                                                  buffer_out_.size()),
                              std::bind(&Connection::OnWritten,
                                        shared_from_this(),
                                        std::placeholders::_1,
                                        std::placeholders::_2));
}

void Connection::ConstructMessage() {
    if (buffer_in_.size() <= 0)
        return;

    if (message_->MessageCompleted()) {
        XERROR << "Message is already complete.";
        return;
    }

    if (!message_->consume(buffer_in_)) {
        XERROR << "Message parse failure.";
        return;
    }

    // TODO should we check if buffer_in_ is empty here?

    if (!message_->MessageCompleted()) {
        read();
        return;
    }
}

void Connection::reset() {
    // TODO should we cancel the timer here?
    timer_triggered_ = false;
    message_->reset();

    if (buffer_in_.size() > 0)
        buffer_in_.consume(buffer_in_.size());

    if (buffer_out_.size() > 0)
        buffer_out_.consume(buffer_out_.size());
}

bool Connection::SessionInvalidated() {
    std::shared_ptr<Session> s(session_.lock());
    if (!s || s->stopped())
        return true;
    return false;
}

void Connection::DestroySession() {
    std::shared_ptr<Session> s(session_.lock());
    if (!s)
        return;
    s->destroy();
}

void Connection::StartTimer() {
    timer_running_ = true;
    timer_.expires_from_now(timeout_);
    auto self(shared_from_this());
    timer_.async_wait([this, self] (const boost::system::error_code& e) {
        timer_running_	= false;
        timer_triggered_ = true;
        OnTimeout(e);
    });
}

void Connection::CancelTimer() {
    if (timer_running_)
        timer_.cancel();
}
