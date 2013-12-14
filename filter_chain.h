#ifndef FILTER_CHAIN_H
#define FILTER_CHAIN_H

#include <list>
#include "connection.h"
#include "filter.h"
#include "filter_context.h"
#include "http_container.h"
#include "resettable.h"

class HttpObject;

class FilterChain : public Resettable {
public:
    typedef std::list<Filter*> container_type;

    FilterChain() : context_(new FilterContext) {}

    virtual ~FilterChain() { // TODO delete all filters?
        if(context_) delete context_;
    }

    void RegisterFilter(Filter *filter);

    void FilterRequest();
    void FilterResponse();

    FilterContext *FilterContext() const { return context_; }

    virtual void reset() {
        context_->reset();
    }

private:
    void AddFilter(container_type& container, Filter *filter);
    void filter(container_type& container);

    container_type request_filters_;
    container_type response_filters_;

    FilterContext *context_;
};

#endif // FILTER_CHAIN_H
