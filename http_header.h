#ifndef HTTP_HEADER_H
#define HTTP_HEADER_H

#include "log.h"


struct HttpHeader {
    std::string name;
    std::string value;

    HttpHeader(const std::string& name = "",
               const std::string& value = "")
        : name(name), value(value) {
        TRACE_THIS_PTR;
    }
    ~HttpHeader() {
        TRACE_THIS_PTR;
    }
};

#endif // HTTP_HEADER_H
