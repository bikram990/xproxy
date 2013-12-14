#ifndef FILTER_CHAIN_H
#define FILTER_CHAIN_H

#include <list>
#include "resettable.h"

class Filter;
class FilterContext;

class FilterChain : public Resettable {
public:
    typedef std::list<Filter*> container_type;

    FilterChain();

    virtual ~FilterChain();

    void RegisterFilter(Filter *filter);

    void FilterRequest();
    void FilterResponse();

    class FilterContext *FilterContext() const { return context_; }

    virtual void reset();

private:
    void AddFilter(container_type& container, Filter *filter);
    void filter(container_type& container);

    container_type request_filters_;
    container_type response_filters_;

    class FilterContext *context_;
};

#endif // FILTER_CHAIN_H
