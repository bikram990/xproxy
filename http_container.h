#ifndef HTTP_CONTAINER_H
#define HTTP_CONTAINER_H

#include <boost/shared_ptr.hpp>
#include "http_headers.h"
#include "http_initial.h"
#include "resettable.h"

class HttpContainer : public Resettable {
public:
    std::size_t size() const {
        return objects_.size();
    }

    void AppendObject(HttpObjectPtr object) {
        objects_.push_back(object);
    }

    HttpObjectPtr RetrieveObject(std::size_t index) {
        if(index > size())
            return HttpObjectPtr();
        return objects_[index];
    }

    HttpInitialPtr RetrieveInitial() {
        auto it = std::find_if(objects_.begin(), objects_.end(),
                               [](HttpObjectPtr object) {
                                   return object->type() == HttpObject::kHttpRequestInitial
                                   || object->type() == HttpObject::kHttpResponseInitial;
                               });

        if(it != objects_.end())
            return std::static_pointer_cast<HttpInitial>(*it);
        return HttpInitialPtr();
    }

    HttpHeadersPtr RetrieveHeaders() {
        auto it = std::find_if(objects_.begin(), objects_.end(),
                               [](HttpObjectPtr object) {
                                   return object->type() == HttpObject::kHttpHeaders;
                               });

        if(it != objects_.end())
            return std::static_pointer_cast<HttpHeaders>(*it);
        return HttpHeadersPtr();
    }

    HttpObjectPtr RetrieveLatest() {
        if(size() <= 0)
            return HttpObjectPtr();
        return objects_[size() - 1];
    }

    virtual void reset() {
        objects_.clear();
    }

private:
    std::vector<HttpObjectPtr> objects_;
};

#endif // HTTP_CONTAINER_H
