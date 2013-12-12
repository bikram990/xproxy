#ifndef HTTP_CHUNK_H
#define HTTP_CHUNK_H

#include "http_object.h"

class HttpChunk : public HttpObject {
public:
    HttpChunk() : HttpObject(), last_(false) {}

    HttpChunk(SharedBuffer buffer) : HttpObject(buffer), last(false) {}

    virtual ~HttpChunk() {}

    void SetLast(bool value) { last_ = value; }
    bool IsLast() const { return last_; }

private:
    virtual void UpdateByteBuffer() {
        // TODO do nothing here
    }

    bool last_; // whether this is the last chunk
};

#endif // HTTP_CHUNK_H
