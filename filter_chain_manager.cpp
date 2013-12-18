#include "builtin_filters.h"
#include "filter.h"
#include "filter_chain.h"
#include "filter_chain_manager.h"

FilterChainManager::FilterChainManager() {
    builtin_filters_.push_back(new EntityCollectorFilter);
    builtin_filters_.push_back(new SSLHandler);
    builtin_filters_.push_back(new ServerConnectionObtainer);
    builtin_filters_.push_back(new UriCanonicalizer);
    builtin_filters_.push_back(new RequestSenderFilter);
    builtin_filters_.push_back(new ResponseReceiverFilter);
}

FilterChainManager::~FilterChainManager() {
    for(auto it = builtin_filters_.begin();
        it != builtin_filters_.end(); ++it)
        if(*it) delete *it;

    for(auto it = chains_.begin(); it!= chains_.end(); ++it)
        if(*it) delete *it;

}
