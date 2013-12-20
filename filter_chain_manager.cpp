#include "builtin_filters.h"
#include "filter.h"
#include "filter_chain.h"
#include "filter_chain_manager.h"

FilterChainManager::FilterChainManager() {
    builtin_filters_.push_back(new DefaultRequestFilter);
    builtin_filters_.push_back(new RequestSenderFilter);
    builtin_filters_.push_back(new DefaultResponseFilter);
}

FilterChainManager::~FilterChainManager() {
    for(auto it = builtin_filters_.begin();
        it != builtin_filters_.end(); ++it)
        if(*it) delete *it;

    for(auto it = chains_.begin(); it!= chains_.end(); ++it)
        if(*it) delete *it;

}
