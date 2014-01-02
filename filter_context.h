#ifndef FILTER_CONTEXT_H
#define FILTER_CONTEXT_H

#include "boost/shared_ptr.hpp"
#include "http_container.h"
#include "log.h"
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

    boost::shared_ptr<Connection> connection() const {
        if(!connection_) {
            XERROR << "The connection has been reset.";
        }
        return connection_;
    }

    boost::shared_ptr<Connection> BridgedConnection() const {
        if(!bridged_connection_) {
            XERROR << "The bridge connection has been reset.";
        }
        return bridged_connection_;
    }

    HttpContainer *container() const {
        if(!container_) {
            XERROR << "The container has been reset.";
        }
        return container_.get();
    }

    /**
     * @brief reset
     *
     * This is the implementation of interface Resettable
     *
     * By default, we do not reset the connection, may be there is filter using
     * the context when we are doing resetting, so, do not reset the connection
     * to avoid program crash.
     */
    virtual void reset() {
        // connection_.reset();
        // bridged_connection_.reset();
        // container_->reset();
        reset(false);
    }

    /**
     * @brief reset
     * @param all
     *
     * Reset the context
     *
     * if all is false, it is the same as reset(), but if it is true, the
     * connection will also be reset. Only call this when to destruct a
     * connection(to decrease the reference count of connection in shared_ptr).
     */
    virtual void reset(bool all) {
        bridged_connection_.reset();
        container_->reset();

        if(all)
            connection_.reset();
    }

protected:
    boost::shared_ptr<Connection> connection_;
    boost::shared_ptr<Connection> bridged_connection_;
    std::unique_ptr<HttpContainer> container_;
};

#endif // FILTER_CONTEXT_H
