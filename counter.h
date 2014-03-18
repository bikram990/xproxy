#ifndef COUNTER_H
#define COUNTER_H

#include <atomic>
#include "common.h"

template<typename T>
class Counter {
public:
    std::size_t id() const { return id_; }

protected:
    Counter() : id_(++counter_) {}
    virtual ~Counter() {}

	DISABLE_COPY_AND_ASSIGNMENT(Counter);

private:
    std::size_t id_;

    static std::atomic<std::size_t> counter_;
};


template<typename T>
std::atomic<std::size_t> Counter<T>::counter_(0);

#endif // COUNTER_H
