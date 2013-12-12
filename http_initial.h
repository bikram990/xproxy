#ifndef HTTP_INITIAL_H
#define HTTP_INITIAL_H

#include "http_object.h"

class HttpInitial : public HttpObject {
public:
    HttpInitial() : modified_(true) {} // TODO true or false?

    virtual ~HttpInitial() {}

    virtual SharedBuffer BinaryContent() {
        if(!modified_)
            return content_;

        UpdateLineString(); // subclass should implement this interface

        content_->reset();
        *content_ << line_;

        modified_ = false;

        return content_;
    }

protected:
    virtual void UpdateLineString() = 0;

    bool modified_;
    std::string line_;
};

#endif // HTTP_INITIAL_H
