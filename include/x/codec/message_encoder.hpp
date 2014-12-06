#ifndef MESSAGE_ENCODER_HPP
#define MESSAGE_ENCODER_HPP

namespace x {
namespace memory { class byte_buffer; }
namespace message { class message; }
namespace codec {

class message_encoder {
public:
    DEFAULT_DTOR(message_encoder);

    virtual std::size_t encode(const message::message& msg, memory::byte_buffer& buf) = 0;

    virtual void reset() = 0;
};

} // namespace codec
} // namespace x

#endif // MESSAGE_ENCODER_HPP
