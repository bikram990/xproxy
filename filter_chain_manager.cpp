#include "entity_collector_filter.h"
#include "filter.h"
#include "filter_chain.h"
#include "filter_chain_manager.h"
#include "request_sender_filter.h"

FilterChainManager::FilterChainManager() {
    predefined_filters_.push_back(new EntityCollectorFilter);
    predefined_filters_.push_back(new RequestSenderFilter);
}

FilterChainManager::~FilterChainManager() {
    for(auto it = predefined_filters_.begin();
        it != predefined_filters_.end(); ++it)
        if(*it) delete *it;

    for(auto it = chains_.begin(); it!= chains_.end(); ++it)
        if(*it) delete *it;

}

FilterChain *FilterChainManager::RequireFilterChain() {
    boost::lock_guard<boost::mutex> lock(lock_);

    if(chains_.size() > 0) {
        FilterChain *chain = chains_.front();
        chains_.pop_front();
        return chain;
    }

    std::unique_ptr<FilterChain> chain(new FilterChain);
    for(auto it = predefined_filters_.begin();
        it != predefined_filters_.end(); ++it)
        chain->RegisterFilter(*it);
    return chain.release();
}

void FilterChainManager::ReleaseFilterChain(FilterChain *chain) {
    if(!chain)
        return;

    chain->reset();

    boost::lock_guard<boost::mutex> lock(lock_);
    chains_.push_front(chain);
}
