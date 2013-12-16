#ifndef FILTER_CHAIN_MANAGER_H
#define FILTER_CHAIN_MANAGER_H

#include <list>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

class Filter;
class FilterChain;
class ProxyServer;

class FilterChainManager : private boost::noncopyable {
    friend class ProxyServer;
public:
    FilterChain *RequireFilterChain();
    void ReleaseFilterChain(FilterChain *chain);

    ~FilterChainManager();

private:
    FilterChainManager();

    std::vector<Filter*> predefined_filters_;
    std::list<FilterChain*> chains_;
    boost::mutex lock_;
};

#endif // FILTER_CHAIN_MANAGER_H
