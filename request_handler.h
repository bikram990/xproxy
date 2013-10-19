#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include "http_request.h"

class HttpProxySession;


class RequestHandler : public boost::enable_shared_from_this<RequestHandler> {
public:
    static RequestHandler *CreateHandler(HttpProxySession& session,
                                         HttpRequestPtr request);
    virtual void HandleRequest() = 0;
    virtual ~RequestHandler();

protected:
    RequestHandler();

private:
    static RequestHandler *CreateHttpHandler(HttpProxySession& session,
                                             HttpRequestPtr request);
};

#endif // REQUEST_HANDLER_H
