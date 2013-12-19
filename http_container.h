#ifndef HTTP_CONTAINER_H
#define HTTP_CONTAINER_H

#include <boost/shared_ptr.hpp>
#include "http_headers.h"
#include "http_initial.h"
#include "resettable.h"

class HttpContainer : public Resettable {
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

    HttpInitial *RetrieveInitial() {
        auto it = std::find_if(objects_.begin(), objects_.end(),
                               [](HttpObject *object) {
                                   return object->type() == HttpObject::kHttpRequestInitial
                                   || object->type() == HttpObject::kHttpResponseInitial;
                               });

        if(it != objects_.end())
            return reinterpret_cast<HttpInitial*>(*it);
        return nullptr;
    }

    HttpHeaders *RetrieveHeaders() {
        auto it = std::find_if(objects_.begin(), objects_.end(),
                               [](HttpObject *object) {
                                   return object->type() == HttpObject::kHttpHeaders;
                               });

        if(it != objects_.end())
            return reinterpret_cast<HttpHeaders*>(*it);
        return nullptr;
    }

    HttpObject *RetrieveLatest() {
        if(size() <= 0)
            return nullptr;
        return objects_[size() - 1];
    }

    virtual void reset() {
        std::for_each(objects_.begin(), objects_.end(),
                      [](HttpObject* object) { if(object) delete object; });
        objects_.clear();
    }

private:
    std::vector<HttpObject*> objects_;
};

#endif // HTTP_CONTAINER_H
