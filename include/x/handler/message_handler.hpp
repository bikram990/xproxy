#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

namespace x {
namespace message { class message; }
namespace handler {

class message_handler {
public:
    DEFAULT_DTOR(message_handler);

    virtual void handle_message(const message::message& msg) = 0;
};

} // namespace handler
} // namespace x

#endif // MESSAGE_HANDLER_HPP
