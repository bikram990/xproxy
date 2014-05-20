#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include <mutex>
#include <thread>
#include "common.hpp"

namespace xproxy {
namespace util {

template<class R>
class Singleton {
public:
    Singleton() : handle_(nullptr) {}
    ~Singleton() { delete handle_; }

    R& get() {
        if(!handle_) {
            std::lock_guard<std::mutex> lock(mutex_);
            if(!handle_)
                handle_ = new R;
        }
        return *handle_;
    }

private:
    std::mutex mutex_;
    R *handle_;
};

} // namespace util
} // namespace xproxy

#endif // SINGLETON_HPP
