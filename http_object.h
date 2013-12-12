#ifndef HTTP_OBJECT_H
#define HTTP_OBJECT_H

#include "byte_convertible.h"

class HttpObject : public virtual ByteConvertible {
public:
    HttpObject() : modified_(true) {} // TODO true or false?

    virtual ~HttpObject() {}

    virtual SharedBuffer BinaryContent() {
        if(!modified_)
            return content_;

        UpdateByteBuffer();
        modified_ = false;

        return content_;
    }

protected:
    virtual void UpdateByteBuffer() = 0;

    bool modified_;
    SharedBuffer content_;
};

#endif // HTTP_OBJECT_H
