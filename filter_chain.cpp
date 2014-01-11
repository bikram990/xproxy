#include <algorithm>
#include "filter.h"
#include "filter_chain.h"
#include "log.h"

void FilterChain::RegisterFilter(Filter *filter) {
    switch(filter->type()) {
    case Filter::kRequest:
        insert(request_filters_, filter);
        break;
    case Filter::kResponse:
        insert(response_filters_, filter);
        break;
    case Filter::kBoth:
        insert(request_filters_, filter);
        insert(response_filters_, filter);
        break;
    default:
        break;
    }
}

HttpContainer *FilterChain::FilterRequest(HttpContainer *container) {
    for(auto it : request_filters_) {
        HttpContainer *out = nullptr;
        Filter::FilterResult result = it->process(container, &out);
        switch(result) {
        case Filter::kSkip:
            XDEBUG << "Filter " << it->name() << " wants to skip.";

            /// skipping other filters means the request will be sent to server,
            /// so the return value should be nullptr
            assert(out == nullptr);
            return nullptr;
        case Filter::kStop:
            XDEBUG << "Filter " << it->name() << " wants to stop.";

            /// stopping means the request will be terminated,
            /// so the return value should be not null
            assert(out != nullptr);
            return out;
        case Filter::kContinue:
            XDEBUG << "Filter " << it->name() << " wants to continue.";
            break;
        default:
            break;
        }
    }

    // if the program goes here, it means all filters are passed
    return nullptr;
}

void FilterChain::FilterResponse(HttpContainer *container) {
    for(auto it : response_filters_) {
        Filter::FilterResult result = it->process(container);
        switch(result) {
        case Filter::kSkip:
        case Filter::kStop:
            XDEBUG << "Filter " << it->name() << " wants to skip or stop.";
            return;
        case Filter::kContinue:
            XDEBUG << "Filter " << it->name() << " wants to continue.";
            break;
        default:
            break;
        }
    }
}

void FilterChain::insert(std::list<Filter *> filters, Filter *filter) {
    for(auto it = filters.begin(); it != filters.end(); ++it) {
        if((*it)->priority() < filter->priority()) {
            filters.insert(it, filter);
            return;
        }
    }

    filters.push_back(filter);
}
