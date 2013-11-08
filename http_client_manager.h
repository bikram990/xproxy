#ifndef HTTP_CLIENT_MANAGER_H
#define HTTP_CLIENT_MANAGER_H

#include <list>
#include <map>
#include <string>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

class HttpClient;
class HttpRequest;
class HttpResponse;

class HttpClientManager : private boost::noncopyable {
public:
    typedef std::pair<std::string, short> key_type; // domain, port
    typedef std::list<std::auto_ptr<HttpClient>> value_type;
    typedef std::map<key_type, value_type> cache_type; // TODO map or hash_map?

    typedef value_type::iterator value_iterator_type;
    typedef cache_type::iterator cache_iterator_type;

    typedef boost::function<void(const boost::system::error_code&)> callback_type;


    void AsyncHandleRequest(HttpRequest *request, HttpResponse *response, callback_type callback);

private:
    HttpClientManager(boost::asio::io_service& fetch_service);
    ~HttpClientManager();

    HttpClient *get(HttpRequest *request);

    boost::asio::io_service& fetch_service_;
    cache_type cache_;
};

#endif // HTTP_CLIENT_MANAGER_H
