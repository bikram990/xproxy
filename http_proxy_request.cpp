#include "http_proxy_request.h"

HttpProxyRequest::HttpProxyRequest() : state_(kRequestStart) {
}

HttpProxyRequest::~HttpProxyRequest() {
}

HttpProxyRequest::BuildResult HttpProxyRequest::BuildFromRaw(char *buffer, std::size_t length) {
    return HttpProxyRequest::kBuildError;
    // TODO implement me
}
