#ifndef MESSAGE_HPP
#define MESSAGE_HPP

namespace xproxy {
namespace memory { class ByteBuffer; }
namespace message {

/**
 * @brief The Message class
 *
 * The generic message interface.
 */
class Message {
public:
    virtual ~Message() {}

    virtual std::size_t serialize(xproxy::memory::ByteBuffer& buffer) const = 0;
};

}
}

#endif // MESSAGE_HPP
