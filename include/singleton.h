#ifndef SINGLETON_H
#define SINGLETON_H

#include <mutex>
#include <thread>
#include "common.h"

template<class R>
class Singleton : private boost::noncopyable {
public:
    Singleton() : handle_(NULL) {}
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

#endif // SINGLETON_H
