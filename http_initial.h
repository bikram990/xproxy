#ifndef HTTP_INITIAL_H
#define HTTP_INITIAL_H

#include "http_object.h"

class HttpInitial : public HttpObject {
public:
    virtual ~HttpInitial() {}

private:
    virtual void UpdateByteBuffer() {
        UpdateLineString(); // subclass should implement this interface

        content_->reset();
        *content_ << line_;
    }

protected:
    virtual void UpdateLineString() = 0;

    std::string line_;
};

#endif // HTTP_INITIAL_H
