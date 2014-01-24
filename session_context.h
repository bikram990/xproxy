#ifndef SESSION_CONTEXT_H
#define SESSION_CONTEXT_H

#include "http_container.h"

struct SessionContext {
    SessionContext() :
        request(new HttpContainer),
        response(new HttpContainer),
        https(false) {}

    ~SessionContext() {
        delete request;
        delete response;
    }

    HttpContainer *request;
    HttpContainer *response;
    std::string host;
    unsigned short port;
    bool https;
};

#endif // SESSION_CONTEXT_H
