#ifndef FILTER_CHAIN_H
#define FILTER_CHAIN_H

#include <list>
#include "filter.h"
#include "filter_context.h"
#include "resettable.h"


class FilterChain : public Resettable {
public:
    FilterChain(Filter::FilterType type)
        : type_(type), context_(new class FilterContext) {}

    virtual ~FilterChain() {
        if(context_) delete context_;
    }

    void RegisterFilter(Filter *filter);

    void filter();

    class FilterContext *FilterContext() const { return context_; }

    virtual void reset() { context_->reset(); }

private:
    Filter::FilterType type_;
    std::list<Filter*> filters_;

    class FilterContext *context_;
};

#endif // FILTER_CHAIN_H
