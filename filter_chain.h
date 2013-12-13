#ifndef FILTER_CHAIN_H
#define FILTER_CHAIN_H

#include <list>
#include "connection.h"
#include "http_container.h"

class Filter;
class HttpObject;

class FilterChain {
public:
    virtual ~FilterChain() {} // TODO delete all filters?

    void RegisterFilter(Filter *filter);

    void filter();

    Connection *ClientConnection() { return client_connection_.get(); }

    Connection *ServerConnection() { return server_connection_.get(); }

    HttpContainer *RequestContainer() { return request_.get(); }

    HttpContainer *ResponseContainer() { return response_.get(); }

private:
    std::list<Filter*> filters_;

    ConnectionPtr client_connection_;
    ConnectionPtr server_connection_;

    HttpContainerPtr request_;
    HttpContainerPtr response_;
};

#endif // FILTER_CHAIN_H
