#ifndef SESSION_CONTEXT_H
#define SESSION_CONTEXT_H

#include "http_container.h"

struct SessionContext {
    SessionContext(std::size_t session_id) :
        id(session_id),
        request(new HttpContainer),
        response(new HttpContainer),
        https(false) {}

    ~SessionContext() {
        delete request;
        delete response;
    }

    std::size_t id;
    HttpContainer *request;
    HttpContainer *response;
    std::string host;
    unsigned short port;
    bool https;
};

#endif // SESSION_CONTEXT_H
