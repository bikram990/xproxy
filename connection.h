#ifndef CONNECTION_H
#define CONNECTION_H

#include <memory>
#include "common.h"
#include "socket.h"

class Connection : public std::enable_shared_from_this<Connection> {
public:
    virtual void start() = 0;
    virtual void close() = 0;
    virtual void reset() = 0;

protected:
    DEFAULT_CTOR_AND_DTOR(Connection);
    DISABLE_COPY_AND_ASSIGNMENT(Connection);

private:
    std::unique_ptr<Socket> socket_;
};

#endif // CONNECTION_H
