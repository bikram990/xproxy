#include "filter_chain.h"
#include "filter_chain_manager.h"

FilterChain *FilterChainManager::RequireFilterChain() {
    boost::lock_guard<boost::mutex> lock(lock_);

    if(chains_.size() > 0) {
        FilterChain *chain = chains_.front();
        chains_.pop_front();
        return chain;
    }

    std::unique_ptr<FilterChain> chain(new FilterChain);
    // TODO register filters here
    return chain.release();
}

void FilterChainManager::ReleaseFilterChain(FilterChain *chain) {
    if(!chain)
        return;

    chain->reset();

    boost::lock_guard<boost::mutex> lock(lock_);
    chains_.push_front(chain);
}
