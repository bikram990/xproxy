#ifndef COUNTER_HPP
#define COUNTER_HPP

#include <atomic>
#include "x/common.hpp"

namespace x {
namespace util {

template<class T>
class counter {
public:
    std::size_t id() const { return id_; }

protected:
    counter() : id_(++counter_) {}

    DEFAULT_DTOR(counter);

private:
    std::size_t id_;

    static std::atomic<std::size_t> counter_;

private:
    #warning a better solution, is to increase id_ when copying
    // but now we just disable copying rudely
    MAKE_NONCOPYABLE(counter);
};

template<class T>
std::atomic<std::size_t> counter<T>::counter_(0);

} // namespace util
} // namespace x

#endif // COUNTER_HPP
