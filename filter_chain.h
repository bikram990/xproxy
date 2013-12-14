#ifndef FILTER_CHAIN_H
#define FILTER_CHAIN_H

#include <list>
#include "connection.h"
#include "filter.h"
#include "http_container.h"

class HttpObject;

class FilterChain {
public:
    typedef std::list<Filter*> container_type;

    virtual ~FilterChain() {} // TODO delete all filters?

    void RegisterFilter(Filter *filter);

    void FilterRequest();
    void FilterResponse();

    Connection *ClientConnection() { return client_connection_.get(); }

    Connection *ServerConnection() { return server_connection_.get(); }

    HttpContainer *RequestContainer() { return request_.get(); }

    HttpContainer *ResponseContainer() { return response_.get(); }

private:
    void AddFilter(container_type& container, Filter *filter);
    void filter(container_type& container);

    container_type request_filters_;
    container_type response_filters_;

    ConnectionPtr client_connection_;
    ConnectionPtr server_connection_;

    HttpContainerPtr request_;
    HttpContainerPtr response_;
};

#endif // FILTER_CHAIN_H
