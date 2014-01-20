#ifndef FILTER_CHAIN_H
#define FILTER_CHAIN_H

#include <list>
#include "filter.h"

class HttpContainer;
struct SessionContext;

class FilterChain {
public:
    virtual ~FilterChain() {
        for(auto it : request_filters_)
            delete it;

        for(auto it : response_filters_)
            delete it;
    }

    template<typename Container>
    void RegisterAll(const Container& filters) {
        std::for_each(filters.begin(), filters.end(),
                      [this](Filter *filter) { RegisterFilter(filter); });
    }

    void RegisterFilter(Filter *filter);

    Filter::FilterResult FilterRequest(SessionContext& context);

    void FilterResponse(SessionContext& context);

private:
    void insert(std::list<Filter*> filters, Filter *filter);

private:
    std::list<Filter*> request_filters_;
    std::list<Filter*> response_filters_;
};

#endif // FILTER_CHAIN_H
