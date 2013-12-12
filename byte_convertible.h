#ifndef BYTE_CONVERTIBLE_H
#define BYTE_CONVERTIBLE_H

#include "byte_buffer.h"

/**
 * @brief The ByteConvertible class
 *
 * Classes inherit this interface should can be converted into binary content
 */
class ByteConvertible {
public:
    virtual ~ByteConvertible() {}

    virtual SharedBuffer BinaryContent() = 0;
};

#endif // BYTE_CONVERTIBLE_H
