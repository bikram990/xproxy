#ifndef ENTITY_COLLECTOR_FILTER_H
#define ENTITY_COLLECTOR_FILTER_H

#include "connection.h"
#include "filter.h"
#include "filter_context.h"
#include "http_container.h"
#include "http_headers.h"

class EntityCollectorFilter : public Filter {
public:
    EntityCollectorFilter() : Filter(kRequest) {}

    virtual FilterResult process(FilterContext *context) {
        HttpContainer *container = context->container();

        for(int i = 0; i < container->size(); ++i) {
            XDEBUG << std::string(container->RetrieveObject(i)->ByteContent()->data(),
                                  container->RetrieveObject(i)->ByteContent()->size());
        }

        if(container->size() < 2) {
            context->connection()->AsyncRead();
            return kStop;
        }

        HttpHeaders *headers = reinterpret_cast<HttpHeaders*>(container->RetrieveObject(1));
        std::string content_length;
        if(headers->find("Content-Length", content_length) && container->size() <= 2) {
            context->connection()->AsyncRead();
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
