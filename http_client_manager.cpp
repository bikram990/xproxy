#include "http_client_manager.h"
#include "http_request.h"

void HttpClientManager::AsyncHandleRequest(HttpProxySession::Mode mode,
                                           HttpRequest *request,
                                           HttpResponse *response,
                                           callback_type callback) {
    HttpClient::Ptr client = get(request);
    if(!client) {
        XERROR << "No http client available.";
        callback(boost::asio::error::invalid_argument);
        return;
    }
    client->AsyncSendRequest(mode, request, response, callback);
}

inline HttpClient::Ptr HttpClientManager::get(HttpRequest *request) {
    CacheKey key(request->host(), request->port());
    cache_iterator_type cit = cache_.find(key);
    if(cit == cache_.end()) {
        boost::lock_guard<boost::mutex> lock(mutex_);
        cit = cache_.find(key);
        if(cit == cache_.end()) {
            HttpClient::Ptr client(HttpClient::Create(fetch_service_));
            XDEBUG << "No http client for [" << request->host() << ":" << request->port()
                   << "] found, new one [id: " << client->id() << "] created.";
            CacheValue value;
            value.clients->push_back(client);
            cache_.insert(std::make_pair(key, value));
            client->state(HttpClient::kInUse);
            return client;
        }
    }

    CacheValue& value = cit->second;
    boost::lock_guard<boost::mutex> lock(*value.mutex);
    value_iterator_type vit = std::find_if(value.clients->begin(), value.clients->end(), AvailabilitySearcher());
    if(vit != value.clients->end()) {
        XDEBUG << "Cached http client " << "[id: " << (*vit)->id() << "]"
               << " for [" << request->host() << ":" << request->port() << "] found.";
        (*vit)->state(HttpClient::kInUse);
        return *vit;
    }

    HttpClient::Ptr client(HttpClient::Create(fetch_service_));
    XDEBUG << "Http clients for [" << request->host() << ":" << request->port()
           << "] are all busy, new one [id: " << client->id() << "] created.";
    value.clients->push_back(client);
    client->state(HttpClient::kInUse);
    return client;
}
