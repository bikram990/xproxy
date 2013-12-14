#ifndef FILTER_CONTEXT_H
#define FILTER_CONTEXT_H

#include "boost/shared_ptr.hpp"
#include "resettable.h"

class Connection;
class HttpContainer;

class FilterContext : public Resettable {
public:
    virtual ~FilterContext() {}

    void SetClientConnection(boost::shared_ptr<Connection> connection) {
        client_connection_ = connection;
    }

    Connection *ClientConnection() const { return client_connection_.get(); }

    void SetServerConnection(boost::shared_ptr<Connection> connection) {
        server_connection_ = connection;
    }

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
    boost::shared_ptr<Connection> client_connection_;
    boost::shared_ptr<Connection> server_connection_;

    boost::shared_ptr<HttpContainer> request_;
    boost::shared_ptr<HttpContainer> response_;
};

#endif // FILTER_CONTEXT_H
