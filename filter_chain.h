#ifndef FILTER_CHAIN_H
#define FILTER_CHAIN_H

#include <list>

class Filter;
class HttpContainer;

class FilterChain {
public:
    virtual ~FilterChain() {
        for(auto it : request_filters_)
            delete *it;

        for(auto it : response_filters_)
            delete *it;
    }

    template<typename Container>
    void RegisterAll(const Container& filters) {
        std::for_each(filters.begin(), filters.end(),
                      [this](Filter *filter) { RegisterFilter(filter); });
    }

    void RegisterFilter(Filter *filter);

    /**
     * Filter http request.
     *
     * @brief FilterRequest
     * @param container         the request to be filtered
     * @return HttpContainer    should be null or a valid http response
     *
     * When the return value is not nullptr, it means the request should not be
     * sent to server, instead, the returned HttpContainer instance should be
     * written to client socket. Otherwise the request should be sent to server.
     */
    HttpContainer *FilterRequest(HttpContainer *container);

    /**
     * Filter http response.
     *
     * @brief FilterResponse
     * @param container         the response to be filtered
     *
     * This is different from FilterResponse(), no matter what the filtering
     * result is, something should be written to client socket. So, if the
     * response needs to be modified, just modify the parameter directly.
     */
    void FilterResponse(HttpContainer *container);

private:
    void insert(std::list<Filter*> filters, Filter *filter);

private:
    std::list<Filter*> request_filters_;
    std::list<Filter*> response_filters_;
};

#endif // FILTER_CHAIN_H
