#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include "x/common.hpp"

namespace x {
namespace message {

class message {
public:
    DEFAULT_DTOR(message);

    virtual bool deliverable() = 0;

    virtual void reset() = 0;
};

} // namespace message
} // namespace x

#endif // MESSAGE_HPP
