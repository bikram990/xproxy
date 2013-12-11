#ifndef FILTER_CHAIN_H
#define FILTER_CHAIN_H

#include <list>
#include "filter.h"

class HttpObject;

class FilterChain {
public:
    void RegisterFilter(const Filter& filter) {
        for(auto it = filters_.begin(); it != filters_.end(); ++it) {
            if(it->priority() >= filter.priority()) { // TODO check if there is bug here
                filters_.insert(it, filter);
                break;
            }
        }
    }

    void filter(const HttpObject& object) {
        for(auto it = filters_.begin(); it != filters_.end(); ++it) {
            Filter::FilterResult result = it->process(object);
            switch(result) {
            case Filter::kSkip:
                // TODO add logic here
                break;
            case Filter::kStop:
                // TODO add logic here
                break;
            case Filter::kContinue:
            default:
                // do nothing here
            }
        }
    }

private:
    std::list filters_;
};

#endif // FILTER_CHAIN_H
