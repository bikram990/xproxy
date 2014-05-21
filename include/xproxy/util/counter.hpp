#ifndef COUNTER_HPP
#define COUNTER_HPP

#include <atomic>
#include "xproxy/common.hpp"

namespace xproxy {
namespace util {

template<class T>
class Counter {
public:
    std::size_t id() const { return id_; }

protected:
    Counter() : id_(++counter_) {}

    DEFAULT_VIRTUAL_DTOR(Counter);

private:
    std::size_t id_;

    static std::atomic<std::size_t> counter_;

private:
    // TODO a better solution, is to increase id_ when copying,
    // but not just disable copying rudely
    MAKE_NONCOPYABLE(Counter);
};

template<class T>
std::atomic<std::size_t> Counter<T>::counter_(0);

} // namespace util
} // namespace xproxy

#endif // COUNTER_HPP
