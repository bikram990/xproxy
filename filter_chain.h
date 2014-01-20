#ifndef FILTER_CHAIN_H
#define FILTER_CHAIN_H

#include <list>
#include "filter.h"

class HttpContainer;
struct SessionContext;

class FilterChain {
public:
    virtual ~FilterChain() {}

    template<typename Container>
    void RegisterAll(const Container& filters) {
        std::for_each(filters.begin(), filters.end(),
                      [this](Filter::Ptr filter) { RegisterFilter(filter); });
    }

    void RegisterFilter(Filter::Ptr filter);

    Filter::FilterResult FilterRequest(SessionContext& context);

    void FilterResponse(SessionContext& context);

private:
    void insert(std::list<Filter::Ptr> filters, Filter::Ptr filter);

private:
    std::list<Filter::Ptr> request_filters_;
    std::list<Filter::Ptr> response_filters_;
};

#endif // FILTER_CHAIN_H
