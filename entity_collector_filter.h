#ifndef ENTITY_COLLECTOR_FILTER_H
#define ENTITY_COLLECTOR_FILTER_H

#include "filter.h"
#include "filter_chain.h"
#include "http_headers.h"
#include "http_object.h"

class EntityCollectorFilter : public Filter {
public:
    EntityCollectorFilter() : Filter(kRequest) {}

    virtual FilterResult process() {
        HttpContainer *container = chain_->RequestContainer();
        if(container->size() < 2) {
            chain_->ClientConnection()->AsyncRead();
            return kStop;
        }

        HttpHeaders *headers = reinterpret_cast<HttpHeader*>(container->RetrieveObject(1));
        std::string content_length;
        if(headers->find("Content-Length", content_length) && container->size() <= 2) {
            chain_->ClientConnection()->AsyncRead();
            return kStop;
        }

        return kContinue;
    }

    virtual int priority() {
        return kHighest;
    }

    virtual const std::string name() const {
        return "EntityCollectorFilter";
    }
};

#endif // ENTITY_COLLECTOR_FILTER_H
