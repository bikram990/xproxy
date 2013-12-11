#ifndef HTTP_OBJECT_H
#define HTTP_OBJECT_H

#include "byte_convertible.h"

class HttpObject : public virtual ByteConvertible {
public:
    virtual ~HttpObject() {}

    virtual SharedBuffer BinaryContent() {
        return content_;
    }

protected:
    SharedBuffer content_;
};

#endif // HTTP_OBJECT_H
