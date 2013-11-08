#include "http_client_manager.h"

HttpClientManager::HttpClientManager(boost::asio::io_service& fetch_service)
    : fetch_service_(fetch_service) {
    TRACE_THIS_PTR;
}

HttpClientManager::~HttpClientManager() {
    TRACE_THIS_PTR;
}

void HttpClientManager::AsyncHandleRequest(HttpRequest *request,
                                           HttpResponse *response,
                                           callback_type callback) {
    HttpClient *client = get(request);
    // TODO implement this function
}

inline HttpClient *HttpClientManager::get(HttpRequest *request) {
    // TODO implement this function
    return NULL;
//    cache_iterator_type it = cache_.find(std::make_pair(request->host(), request->port()));
//    if(it == cache_.end()) {
//        std::auto_ptr<HttpClient> client(new HttpClient());
//        std::pair key(request->host(), request->port());
//        std::list value;
//        value.push_front(client);
//        cache_.insert(std::make_pair(key, value));
//        return client.get();
//    }

//    std::list& clients = it->second;
}
