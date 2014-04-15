#include "connection.h"
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
      buffer_size_(buffer_size),
      out_(new boost::asio::streambuf),
      aux_out_(new boost::asio::streambuf) {}
//      out_(std::make_unique<boost::asio::streambuf>()),
//      aux_out_(std::make_unique<boost::asio::streambuf>()) {}

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

void Connection::write(std::shared_ptr<HttpMessage> message) {
    std::lock_guard<std::mutex> lock(lock_);
    bool owner = out_->size() <= 0;
    auto buf = owner ? out_.get() : aux_out_.get();
    if (!message->serialize(*buf)) {
        XERROR << "Error occurred during serialization message.";
        DestroySession();
        return;
    }

    const static std::string msg1("write() called, will start a write operation.");
    const static std::string msg2("write() called, a write operation exists, just write data to out buffer.");
    XDEBUG << (owner ? msg1 : msg2);

    if (owner)
        write();
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

void Connection::write() {
    if (!connected_) {
        connect();
        return;
    }
    socket_->async_write_some(boost::asio::buffer(out_->data(), out_->size()),
                              std::bind(&Connection::OnWritten,
                                        shared_from_this(),
                                        std::placeholders::_1,
                                        std::placeholders::_2));
}

void Connection::reset() {
    // TODO should we cancel the timer here?
    timer_triggered_ = false;
    message_->reset();

    if (buffer_in_.size() > 0)
        buffer_in_.consume(buffer_in_.size());

    if (out_->size() > 0)
        out_->consume(out_->size());
    if (aux_out_->size() > 0)
        aux_out_->consume(aux_out_->size());
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
