#ifndef TIMER_HPP
#define TIMER_HPP

#include <boost/asio.hpp>
#include "xproxy/log/log.hpp"

namespace xproxy {
namespace util {

/**
 * @brief The Timer class
 *
 * This class is a simple wrapper for timer in boost asio.
 */
class Timer {
public:
    Timer(boost::asio::io_service& service)
        : timer_(service),
          running_(false),
          triggered_(false) {}

    ~Timer() = default;

    bool running() const { return running_; }
    bool triggered() const { return triggered_; }

    template<typename TimeoutHandler>
    void start(long timeout, TimeoutHandler&& handler) {
        if (running_)
            return;
        timer_.expires_from_now(boost::posix_time::seconds(timeout));
        timer_.async_wait([this, handler] (const boost::system::error_code& e) {
            running_ = false;

            if (e == boost::asio::error::operation_aborted)
                XDEBUG << "Timer cancelled.";
            else if (e)
                XERROR << "Timer error, message: " << e.message();
            else if (timer_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
                triggered_ = true;
                handler(e);
            }
        });
        running_ = true;
    }

    void cancel() {
        if (running_)
            timer_.cancel();
    }

private:
    bool running_;
    bool triggered_;
    boost::asio::deadline_timer timer_;
};

} // namespace util
} // namespace xproxy

#endif // TIMER_HPP
