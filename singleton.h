#ifndef SINGLETON_H
#define SINGLETON_H

#include <boost/thread.hpp>
#include "common.h"

template<class R>
class Singleton : private boost::noncopyable {
public:
    Singleton() : handle_(NULL) {}
    ~Singleton() { CLEAN_MEMBER(handle_); }

    R& get() {
        if(!handle_) {
            boost::lock_guard<boost::mutex> lock(mutex_);
            if(!handle_)
                handle_ = new R;
        }
        return *handle_;
    }

private:
    boost::mutex mutex_;
    R *handle_;
};

#endif // SINGLETON_H
