#ifndef SERVER_H
#define SERVER_H

#include <boost/noncopyable.hpp>

class Server : private boost::noncopyable {
public:
    void Start() = 0;
    void Stop() = 0;

    ~Server() = 0;
protected:
    Server();
};

#endif // SERVER_H
