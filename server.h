#ifndef SERVER_H
#define SERVER_H

#include <boost/noncopyable.hpp>

class Server : private boost::noncopyable {
public:
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual ~Server() = 0;

protected:
    Server();
};

#endif // SERVER_H
