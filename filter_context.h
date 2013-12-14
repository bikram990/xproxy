#ifndef FILTER_CONTEXT_H
#define FILTER_CONTEXT_H

#include <boost/shared_ptr.hpp>
#include "connection.h"
#include "http_container.h"
#include "resettable.h"

class FilterContext : public Resettable {
public:
    virtual ~FilterContext() {}

    Connection *ClientConnection() const { return client_connection_.get(); }

    Connection *ServerConnection() const { return server_connection_.get(); }

    HttpContainer *RequestContainer() const { return request_.get(); }

    HttpContainer *ResponseContainer() const { return response_.get(); }

    virtual void reset() {
        client_connection_.reset();
        server_connection_.reset();
        request_.reset();
        response_.reset();
    }

protected:
    ConnectionPtr client_connection_;
    ConnectionPtr server_connection_;

    HttpContainerPtr request_;
    HttpContainerPtr response_;
};

typedef boost::shared_ptr<FilterContext> FilterContextPtr;

#endif // FILTER_CONTEXT_H
