#include "http_proxy_request.h"

HttpProxyRequest::HttpProxyRequest() : state_(kRequestStart) {
}

HttpProxyRequest::~HttpProxyRequest() {
}

BuildResult BuildFromRaw(IterT begin, IterT end) {
    return kNotStart;
    // TODO implement me
}
