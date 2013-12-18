#ifndef FILTER_CONTEXT_H
#define FILTER_CONTEXT_H

#include "boost/shared_ptr.hpp"
#include "http_container.h"
#include "resettable.h"

class Connection;

class FilterContext : public Resettable {
public:
    FilterContext() : container_(new HttpContainer) {}

    virtual ~FilterContext() {}

    void SetConnection(boost::shared_ptr<Connection> connection) {
        connection_ = connection;
    }

    void SetBridgedConnection(boost::shared_ptr<Connection> connection) {
        bridged_connection_ = connection;
    }

    boost::shared_ptr<Connection> connection() const { return connection_; }

    boost::shared_ptr<Connection> BridgedConnection() const { return bridged_connection_; }

    HttpContainer *container() const { return container_.get(); }

    virtual void reset() {
        container_.reset(new HttpContainer);
    }

protected:
    boost::shared_ptr<Connection> connection_;
    boost::shared_ptr<Connection> bridged_connection_;
    std::unique_ptr<HttpContainer> container_;
};

#endif // FILTER_CONTEXT_H
