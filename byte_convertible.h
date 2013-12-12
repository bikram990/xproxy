#ifndef BYTE_CONVERTIBLE_H
#define BYTE_CONVERTIBLE_H

#include "byte_buffer.h"

/**
 * @brief The ByteConvertible class
 *
 * Classes inherit from this interface should can be converted into byte content
 */
class ByteConvertible {
public:
    virtual ~ByteConvertible() {}

    virtual SharedBuffer ByteContent() = 0;
};

#endif // BYTE_CONVERTIBLE_H
