#include "filter.h"
#include "filter_chain.h"

void FilterChain::RegisterFilter(Filter *filter) {
    filter->SetFilterChain(this);

    for(auto it = filters_.begin(); it != filters_.end(); ++it) {
        if((*it)->priority() < filter->priority()) { // TODO check if there is bug here
            filters_.insert(it, filter);
            return;
        }
    }

    filters_.push_back(filter);
}

void FilterChain::filter() {
    // TODO enhance the logic here
    for(auto it = filters_.begin(); it != filters_.end(); ++it) {
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
