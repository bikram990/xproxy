#include "filter.h"
#include "filter_chain.h"
#include "filter_context.h"
#include "log.h"

FilterChain::FilterChain() : context_(new class FilterContext) {}

FilterChain::~FilterChain() { // TODO delete all filters?
    if(context_) delete context_;
}

void FilterChain::reset() {
    context_->reset();
}

void FilterChain::RegisterFilter(Filter *filter) {
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
            container.insert(it, filter);
            return;
        }
    }

    container.push_back(filter);
}

void FilterChain::filter(container_type &container) {
    // TODO enhance the logic here
    for(auto it = container.begin(); it != container.end(); ++it) {
        Filter::FilterResult result = (*it)->process(context_);
        switch(result) {
        // for skip and stop, we just return directly
        case Filter::kSkip:
        case Filter::kStop:
            return;
        // for continue, we continue the filtering
        case Filter::kContinue:
        default:
            // do nothing here
            ; // add the comma to pass the compilation
        }
    }
}
