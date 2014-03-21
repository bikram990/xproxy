#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

#include "boost/asio.hpp"

class Serializable {
public:
    virtual ~Serializable() {}

    virtual bool serialize(boost::asio::streambuf& out_buffer) = 0;
};

#endif // SERIALIZABLE_H
