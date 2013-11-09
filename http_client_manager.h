#ifndef HTTP_CLIENT_MANAGER_H
#define HTTP_CLIENT_MANAGER_H

#include "http_client.h"
#include "log.h"
#include "singleton.h"

class HttpRequest;
class HttpResponse;

class HttpClientManager : private boost::noncopyable {
    friend class Singleton<HttpClientManager>;
private:
    struct CacheKey {
        std::string domain;
        short port;
        CacheKey(const std::string& domain = "", short port = 80)
            : domain(domain), port(port) {}
    };

    struct CacheValue {
        boost::mutex *mutex;
        std::vector<HttpClient::Ptr> *clients;
        CacheValue()
            : mutex(new boost::mutex),
              clients(new std::vector<HttpClient::Ptr>()) {}
        ~CacheValue() { delete mutex; delete clients; }
    };

    struct CacheKeyComparator { // the customized comparator function for cached key
        bool operator()(const CacheKey& lhs, const CacheKey& rhs) {
            return lhs.domain < rhs.domain && lhs.port < rhs.port;
        }
    };

    struct AvailabilitySearcher {
        bool operator()(const HttpClient::Ptr& client) {
            return client->state() == HttpClient::kAvailable;
        }
    };

    typedef std::map<CacheKey, CacheValue, CacheKeyComparator> cache_type; // TODO map or hash_map?
    typedef std::vector<HttpClient::Ptr>::iterator value_iterator_type;
    typedef cache_type::iterator cache_iterator_type;

    typedef boost::function<void(const boost::system::error_code&)> callback_type;

public:
    static HttpClientManager& instance();

    static void InitFetchService(boost::asio::io_service *service) {
        if(!instance().fetch_service_)
            instance().fetch_service_ = service;
    }

    static void AsyncHandleRequest(HttpProxySession::Mode mode,
                                   HttpRequest *request,
                                   HttpResponse *response,
                                   callback_type callback);

private:
    HttpClientManager() : fetch_service_(NULL) { TRACE_THIS_PTR; }
    DECLARE_GENERAL_DESTRUCTOR(HttpClientManager)

    HttpClient::Ptr get(HttpRequest *request);

    boost::asio::io_service *fetch_service_;
    boost::mutex mutex_;
    cache_type cache_;
};

#endif // HTTP_CLIENT_MANAGER_H
