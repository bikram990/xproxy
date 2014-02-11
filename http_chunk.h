#ifndef HTTP_CHUNK_H
#define HTTP_CHUNK_H

#include "http_object.h"

class HttpChunk : public HttpObject {
public:
    HttpChunk() : HttpObject(kHttpChunk), last_(false) {}

    HttpChunk(SharedByteBuffer buffer) : HttpObject(buffer), last_(false) {}

    virtual ~HttpChunk() {}

    HttpChunk& operator<<(boost::asio::streambuf& buffer) {
        *content_ << buffer;
        return *this;
    }

    void SetLast(bool value) { last_ = value; }
    bool IsLast() const { return last_; }

private:
    virtual void UpdateByteBuffer() {
        // TODO do nothing here
    }

    bool last_; // whether this is the last chunk
};

typedef std::shared_ptr<HttpChunk> HttpChunkPtr;

#endif // HTTP_CHUNK_H
