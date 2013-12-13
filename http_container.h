#ifndef HTTP_CONTAINER_H
#define HTTP_CONTAINER_H

#include <boost/shared_ptr.hpp>
#include "http_object.h"

class HttpContainer {
public:
    virtual ~HttpContainer() {
        for(auto it = objects_.begin(); it != objects_.end(); ++it)
            if(*it) delete *it;
    }

    std::size_t size() const {
        return objects_.size();
    }

    void AppendObject(HttpObject *object) {
        objects_.push_back(object);
    }

    HttpObject *RetrieveObject(std::size_t index) {
        if(index > size())
            return nullptr;
        return objects_[index];
    }

private:
    std::vector<HttpObject*> objects_;
};

typedef boost::shared_ptr<HttpContainer> HttpContainerPtr;

#endif // HTTP_CONTAINER_H
