#ifndef HTTP_CLIENT_MANAGER_H
#define HTTP_CLIENT_MANAGER_H

#include <boost/thread.hpp>
#include "http_client.h"
#include "log.h"

class HttpRequest;
class HttpResponse;
class ProxyServer;

class HttpClientManager : private boost::noncopyable {
    friend class ProxyServer;
private:
    struct CacheKey {
        std::string domain;
        short port;
        CacheKey(const std::string& domain = "", short port = 80)
            : domain(domain), port(port) {}
    };

    struct CacheValue {
        boost::shared_ptr<boost::mutex> mutex;
        boost::shared_ptr<std::vector<HttpClient::Ptr> > clients;
        CacheValue()
            : mutex(new boost::mutex),
              clients(new std::vector<HttpClient::Ptr>) {}
    };

    struct CacheKeyComparator { // the customized comparator function for cached key
        bool operator()(const CacheKey& lhs, const CacheKey& rhs) {
            if(lhs.domain != rhs.domain)
                return lhs.domain < rhs.domain;
            else
                return lhs.port < rhs.port;
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
    void AsyncHandleRequest(HttpProxySession::Mode mode,
                            HttpRequest *request,
                            HttpResponse *response,
                            callback_type callback);

    DECLARE_GENERAL_DESTRUCTOR(HttpClientManager)

private:
    HttpClientManager(boost::asio::io_service& service)
        : fetch_service_(service) { TRACE_THIS_PTR; }

    HttpClient::Ptr get(HttpRequest *request);

    boost::asio::io_service& fetch_service_;
    boost::mutex mutex_;
    cache_type cache_;
};

#endif // HTTP_CLIENT_MANAGER_H
