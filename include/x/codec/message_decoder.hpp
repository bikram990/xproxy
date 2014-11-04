#ifndef MESSAGE_DECODER_HPP
#define MESSAGE_DECODER_HPP

#include "x/common.hpp"

namespace x {
namespace message { class message; }
namespace codec {

class message_decoder {
public:
    DEFAULT_DTOR(message_decoder);

    virtual std::size_t decode(const char *begin, std::size_t length, message::message& msg) = 0;

    virtual void reset() = 0;
};

} // namespace codec
} // namespace x

#endif // MESSAGE_DECODER_HPP
