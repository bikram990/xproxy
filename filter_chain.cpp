#include "filter.h"
#include "filter_chain.h"
#include "log.h"

void FilterChain::RegisterFilter(Filter *filter) {
    filter->SetFilterChain(this);

    switch(filter->type()) {
    case Filter::kRequest:
        AddFilter(request_filters_, filter);
        break;
    case Filter::kResponse:
        AddFilter(response_filters_, filter);
        break;
    case Filter::kBoth:
        AddFilter(request_filters_, filter);
        AddFilter(response_filters_, filter);
        break;
    default:
        XERROR << "Invalid filter type, name: " << filter->name();
    }
}

void FilterChain::FilterRequest() {
    filter(request_filters_);
}

void FilterChain::FilterResponse() {
    filter(response_filters_);
}

void FilterChain::AddFilter(container_type& container, Filter *filter) {
    for(auto it = container.begin(); it != container.end(); ++it) {
        if((*it)->priority() < filter->priority()) { // TODO check if there is bug here
            filters_.insert(it, filter);
            return;
        }
    }

    filters_.push_back(filter);
}

void FilterChain::filter(container_type &container) {
    // TODO enhance the logic here
    for(auto it = container.begin(); it != con.end(); ++it) {
        Filter::FilterResult result = (*it)->process();
        switch(result) {
        // for skip and stop, we just return directly
        case Filter::kSkip:
        case Filter::kStop:
            return;
        // for continue, we continue the filtering
        case Filter::kContinue:
        default:
            // do nothing here
        }
    }
}
