#ifndef FILTER_CHAIN_MANAGER_H
#define FILTER_CHAIN_MANAGER_H

#include <list>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

class FilterChain;
class ProxyServer;

class FilterChainManager : private boost::noncopyable {
    friend class ProxyServer;
public:
    FilterChain *RequireFilterChain();
    void ReleaseFilterChain(FilterChain *chain);

    ~FilterChainManager() {
        for(auto it = chains_.begin(); it!= chains_.end(); ++it)
            if(*it) delete *it;
    }

private:
    FilterChainManager() {}

    std::list<FilterChain*> chains_;
    boost::mutex lock_;
};

#endif // FILTER_CHAIN_MANAGER_H
