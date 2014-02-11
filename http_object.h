#ifndef HTTP_OBJECT_H
#define HTTP_OBJECT_H

#include "byte_convertible.h"

class HttpObject : public virtual ByteConvertible {
public:
    enum Type {
        kHttpObject = 0, kHttpInitial, kHttpHeaders, kHttpChunk,
        kHttpRequestInitial, kHttpResponseInitial
    };

    HttpObject(Type type = kHttpObject) : type_(type), modified_(true), content_(new ByteBuffer) {}

    HttpObject(SharedByteBuffer buffer, Type type = kHttpObject)
        : type_(type), modified_(true), content_(buffer) {}

    virtual ~HttpObject() {}

    virtual SharedByteBuffer ByteContent() {
        if(!modified_)
            return content_;

        UpdateByteBuffer();
        modified_ = false;

        return content_;
    }

    virtual Type type() const { return type_; }

protected:
    virtual void UpdateByteBuffer() = 0;

    Type type_;
    bool modified_;
    SharedByteBuffer content_;
};

typedef std::shared_ptr<HttpObject> HttpObjectPtr;

#endif // HTTP_OBJECT_H
