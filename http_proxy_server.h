#ifndef HTTP_PROXY_SERVER_H
#define HTTP_PROXY_SERVER_H

#include "server.h"


class HttpProxyServer : public Server {
public:
    HttpProxyServer();
    virtual ~HttpProxyServer();

    virtual void Start();
    virtual void Stop();
};

#endif // HTTP_PROXY_SERVER_H
