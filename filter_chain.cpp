#include <algorithm>
#include "filter.h"
#include "filter_chain.h"
#include "log.h"
#include "session.h"

void FilterChain::RegisterFilter(Filter::Ptr filter) {
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

Filter::FilterResult FilterChain::FilterRequest(SessionContext& context) {
    for(auto it : request_filters_) {
        Filter::FilterResult result = it->process(context, Filter::kRequest);
        if(result == Filter::kSkip || result == Filter::kStop) {
            XDEBUG << "Filter " << it->name() << " wants to stop or skip.";
            return result;
        }

        XDEBUG << "Filter " << it->name() << " wants to continue.";
    }

    // if the program goes here, it means all filters are passed
    return Filter::kContinue;
}

void FilterChain::FilterResponse(SessionContext& context) {
    for(auto it : response_filters_) {
        Filter::FilterResult result = it->process(context, Filter::kResponse);
        if(result == Filter::kSkip || result == Filter::kStop) {
            XDEBUG << "Filter " << it->name() << " wants to stop or skip.";
            return;
        }

        XDEBUG << "Filter " << it->name() << " wants to continue.";
    }
}

void FilterChain::insert(std::list<Filter::Ptr> filters, Filter::Ptr filter) {
    for(auto it = filters.begin(); it != filters.end(); ++it) {
        if((*it)->priority() < filter->priority()) {
            filters.insert(it, filter);
            return;
        }
    }

    filters.push_back(filter);
}
