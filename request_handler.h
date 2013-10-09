#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <boost/shared_ptr.hpp>
#include "http_request.h"

class HttpProxySession;


class RequestHandler {
public:
    typedef HttpRequest::State ResultType;
    typedef boost::shared_ptr<HttpRequest> HttpRequestPtr;

    static RequestHandler *CreateHandler(char *data, std::size_t size,
                                         HttpProxySession& session);
    virtual void HandleRequest() = 0;
    virtual ~RequestHandler();

protected:
    RequestHandler();

private:
    static RequestHandler *CreateHttpHandler(char *data, std::size_t size,
                                             HttpProxySession& session);
};

#endif // REQUEST_HANDLER_H
