#ifndef HTTP_INITIAL_H
#define HTTP_INITIAL_H

#include "http_object.h"

class HttpInitial : public HttpObject {
public:
    HttpInitial(HttpObject::Type type = kHttpInitial)
        : HttpObject(kHttpInitial), major_version_(0), minor_version_(0) {}

    virtual ~HttpInitial() {}

    int GetMajorVersion() const { return major_version_; }

    void SetMajorVersion(int version) {
        modified_ = true;
        major_version_ = version;
    }

    int GetMinorVersion() const { return minor_version_; }

    void SetMinorVersion(int version) {
        modified_ = true;
        minor_version_ = version;
    }

private:
    virtual void UpdateByteBuffer() {
        UpdateLineString(); // subclass should implement this interface

        content_->reset();
        *content_ << line_;
    }

protected:
    virtual void UpdateLineString() = 0;

    int major_version_;
    int minor_version_;
    std::string line_;
};

#endif // HTTP_INITIAL_H
