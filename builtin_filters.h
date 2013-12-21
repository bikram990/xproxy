#ifndef BUILTIN_FILTERS_H
#define BUILTIN_FILTERS_H

#include "boost/lexical_cast.hpp"
#include "connection.h"
#include "filter.h"
#include "filter_context.h"
#include "http_chunk.h"
#include "http_headers.h"
#include "http_request_initial.h"
#include "log.h"
#include "proxy_server.h"
#include "server_connection_manager.h"

class DefaultRequestFilter : public Filter {
public:
    static const int kPriority = kMiddle;

    DefaultRequestFilter() : Filter(kRequest) {}

    virtual FilterResult process(FilterContext *context) {
        XDEBUG << context->connection()->identifier()
               << " Filter [" << name() << "] is called, start to processing...";

        std::string request;
        for(std::size_t i = 0; i < context->container()->size(); ++i) {
            HttpObject *object = context->container()->RetrieveObject(i);
            request.append(std::string(object->ByteContent()->data(),
                                       object->ByteContent()->size()));
        }

        XDEBUG << context->connection()->identifier()
               << " Dump http request object:\n"
               << "==================== begin ====================" << '\n'
               << request
               << '\n' << "===================== end =====================";

        // TODO the following variable should depend on something
        bool need_proxy = false;
        bool https = false;

        HttpInitial *initial = context->container()->RetrieveInitial();

        if(!initial) {
            XERROR << "Invalid request, but this should never happen.";
            // TODO write bad request back to client
            return kStop;
        }

        HttpHeaders *headers = context->container()->RetrieveHeaders();

        if(!headers) {
            XERROR << "No headers found in request, but this should never happen.";
            // TODO write bad request back to client
            return kStop;
        }

        if(initial->type() != HttpObject::kHttpRequestInitial) {
            XERROR << "Incorrect request object type: " << initial->type();
            // TODO write bad request back to client
            return kStop;
        }

        HttpRequestInitial *ri = reinterpret_cast<HttpRequestInitial*>(initial);

        std::string host;
        unsigned short port;
        if(need_proxy) {
            // TODO get proxy host port here
            // host = ...
            // port = ...
        } else {
            /// for CONNECT method, we get host, port from the initial line
            /// for other methods, we get host, port from header "Host"
            if(ri->method() == "CONNECT") {
                https = true;

                std::string::size_type sep = ri->uri().find(':');
                if(sep != std::string::npos) {
                    port = boost::lexical_cast<unsigned short>(ri->uri().substr(sep + 1));
                    host = ri->uri().substr(0, sep);
                } else {
                    port = 443;
                    host = ri->uri();
                }
            } else {
                if(!headers->find("Host", host)) {
                    XERROR << "No host found in header, but this should never happen.";
                    // TODO write bad request back to client
                    return kStop;
                }

                std::string::size_type sep = host.find(':');
                if(sep != std::string::npos) {
                    port = boost::lexical_cast<unsigned short>(host.substr(sep + 1));
                    host = host.substr(0, sep);
                } else {
                    port = 80;
                }
            }
        }

        /// Now we set the bridged server connection, if it is set, it should
        /// be a https connection, and it is set during the "CONNECT" request
        ConnectionPtr connection;
        if(context->BridgedConnection()) {
            XDEBUG << "The server connection is set, seems to be a https connection.";
            connection = context->BridgedConnection();
        } else {
            connection = ProxyServer::ServerConnectionManager().RequireConnection(host, port);
            connection->FilterContext()->SetBridgedConnection(context->connection());
            context->SetBridgedConnection(connection);
        }

        /// we use HTTPS mode when we use a proxy
        if(https || need_proxy) {
            connection->PostSSLInitTask();
        }

        /// if it is HTTPS mode, we continue to read from client socket, so
        /// we stop here
        if(https) {
            context->connection()->PostSSLInitTask();
            return kStop;
        }

        /// so the request is not a "CONNECT" request, then we canonicalize
        /// the URI, from "http://host/uri" to "/uri"
        if(ri->uri()[0] != '/') { // the URI is not canonicalized
            std::string http("http://");
            std::string::size_type end = std::string::npos;

            if(ri->uri().compare(0, http.length(), http) != 0) {
                end = ri->uri().find('/');
            } else {
                end = ri->uri().find('/', http.length());
            }

            if(end != std::string::npos) {
                ri->uri().erase(0, end);
            } else {
                XDEBUG << "No host end / found, consider as root: " << ri->uri();
                ri->uri() = '/';
            }
        }

        /// next we build the proxy request if it is needed
        if(need_proxy) {
            // TODO build proxy request here
        }

        return kContinue;
    }

    virtual int priority() {
        /// we only set the default request filter to middle priority, for some
        /// real emergency filters, they can be set to higher priority
        return kPriority;
    }

    virtual const std::string name() const {
        return "DefaultRequestFilter";
    }
};

class EntityCollectorFilter : public Filter {
public:
    EntityCollectorFilter() : Filter(kRequest) {}

    virtual FilterResult process(FilterContext *context) {
        XDEBUG << "In filter: " << name();

        HttpContainer *container = context->container();

        XDEBUG << "Dump http request object:\n"
               << "==================== begin ====================" << '\n'
               << std::string(container->RetrieveLatest()->ByteContent()->data(),
                              container->RetrieveLatest()->ByteContent()->size())
               << '\n' << "===================== end =====================";

        HttpHeaders *headers = container->RetrieveHeaders();

        if(!headers) {
            context->connection()->PostAsyncReadTask();
            return kStop;
        }

        std::string content_length;
        if(headers->find("Content-Length", content_length) && container->size() <= 2) {
            context->connection()->PostAsyncReadTask();
            return kStop;
        }

        return kContinue;
    }

    virtual int priority() {
        return kHighest;
    }

    virtual const std::string name() const {
        return "EntityCollectorFilter";
    }
};

class SSLHandler : public Filter {
public:
    SSLHandler() : Filter(kRequest) {}

    virtual FilterResult process(FilterContext *context) {
        XDEBUG << "In filter: " << name();

        HttpInitial *initial = context->container()->RetrieveInitial();
        if(!initial) {
            XERROR << "Invalid pointer, current container size: " << context->container()->size();
            return kStop;
        }
        if(initial->type() != HttpObject::kHttpRequestInitial) {
            XERROR << "Incorrect object type: " << initial->type();
            return kStop;
        }

        HttpRequestInitial *ri = reinterpret_cast<HttpRequestInitial*>(initial);

        if(ri->method() != "CONNECT") {
            XDEBUG << "Not a Https request, continue.";
            return kContinue;
        }

        // for ssl connections, we set the host and port here
        std::string host;
        unsigned short port = 443;
        std::string::size_type sep = ri->uri().find(':');
        if(sep != std::string::npos) {
            port = boost::lexical_cast<unsigned short>(ri->uri().substr(sep + 1));
            host = ri->uri().substr(0, sep);
        }

        ConnectionPtr connection = ProxyServer::ServerConnectionManager().RequireConnection(host, port);
        connection->FilterContext()->SetBridgedConnection(context->connection());
        context->SetBridgedConnection(connection);

        context->connection()->PostSSLInitTask();
        connection->PostSSLInitTask();

        return kStop;
    }

    virtual int priority() {
        return kHighest - 1;
    }

    virtual const std::string name() const {
        return "SSLHandler";
    }
};

class ServerConnectionObtainer : public Filter {
public:
    ServerConnectionObtainer() : Filter(kRequest) {}

    virtual FilterResult process(FilterContext *context) {
        XDEBUG << "In filter: " << name();

        if(context->BridgedConnection()) {
            XDEBUG << "The server connection is set, seems to be a https connection.";
            return kContinue;
        }

        HttpContainer *container = context->container();
        HttpHeaders *headers = container->RetrieveHeaders();

        if(!headers) {
            // TODO enhance the logic here
            XERROR << "No header object, this should never happen.";
            return kStop;
        }

        std::string host;
        unsigned short port = 80;
        if(!headers->find("Host", host)) {
            XERROR << "no host found, this should never happen.";
            return kStop;
        }

        std::string::size_type sep = host.find(':');
        if(sep != std::string::npos) {
            port = boost::lexical_cast<unsigned short>(host.substr(sep + 1));
            host = host.substr(0, sep);
        }

        XDEBUG << "Remote host and port of current request are: "
               << host << ", " << port << ".";

        ConnectionPtr connection = ProxyServer::ServerConnectionManager().RequireConnection(host, port);
        connection->FilterContext()->SetBridgedConnection(context->connection());
        context->SetBridgedConnection(connection);
        connection->start();
        return kContinue;
    }

    virtual int priority() {
        return kHighest - 2;
    }

    virtual const std::string name() const {
        return "ServerConnectionObtainer";
    }
};

class UriCanonicalizer : public Filter {
public:
    UriCanonicalizer() : Filter(kRequest) {}

    virtual FilterResult process(FilterContext *context) {
        HttpInitial *initial = context->container()->RetrieveInitial();
        if(!initial) {
            XERROR << "Invalid pointer, current container size: " << context->container()->size();
            return kStop;
        }
        if(initial->type() != HttpObject::kHttpRequestInitial) {
            XERROR << "Incorrect object type: " << initial->type();
            return kStop;
        }

        HttpRequestInitial *ri = reinterpret_cast<HttpRequestInitial*>(initial);

        if(ri->method() == "CONNECT") {
            XDEBUG << "Https request, skip.";
            return kContinue;
        }

        if(ri->uri()[0] == '/') { // already canonicalized
            return kContinue;
        }

        std::string http("http://");
        std::string::size_type end = std::string::npos;
        if(ri->uri().compare(0, http.length(), http) != 0)
            end = ri->uri().find('/');
        else
            end = ri->uri().find('/', http.length());

        if(end == std::string::npos) {
            XDEBUG << "No host end / found, consider as root: " << ri->uri();
            ri->uri() = '/';
            return kContinue;
        }

        ri->uri().erase(0, end);
        return kContinue;
    }

    virtual int priority() {
        return kHighest - 3;
    }

    virtual const std::string name() const {
        return "UriCanonicalizer";
    }
};

class RequestSenderFilter : public Filter {
public:
    static const int kPriority = kLowest;

    RequestSenderFilter() : Filter(kRequest) {}

    virtual FilterResult process(FilterContext *context) {
        XDEBUG << context->connection()->identifier()
               << " Filter [" << name() << "] is called, start to processing...";

        boost::shared_ptr<std::vector<SharedBuffer>> buffers(new std::vector<SharedBuffer>);
        HttpContainer *container = context->container();
        for(std::size_t i = 0; i < container->size(); ++i) {
            HttpObject *object = container->RetrieveObject(i);
            buffers->push_back(object->ByteContent());
        }
        context->BridgedConnection()->PostAsyncWriteTask(buffers);
        return kContinue;
    }

    virtual int priority() {
        /// we set this filter to lowest priority, as this is the last filter,
        /// so all other filters should be placed before this filter
        return kPriority;
    }

    virtual const std::string name() const {
        return "RequestSenderFilter";
    }
};

class DefaultResponseFilter: public Filter {
public:
    static const int kPriority = kMiddle;

    DefaultResponseFilter() : Filter(kResponse) {}

    virtual FilterResult process(FilterContext *context) {
        XDEBUG << context->connection()->identifier()
               << " Filter [" << name() << "] is called, start to processing...";

        // TODO the variable below should be set according to something
        bool proxy_used = false;

        HttpObject *latest = context->container()->RetrieveLatest();

        if(!latest) {
            XERROR << "Invalid object, this should never happen.";
            // TODO add logic here
            return kStop;
        }

        XDEBUG << context->connection()->identifier()
               << " Dump http response object:\n"
               << "==================== begin ====================" << '\n'
               << std::string(latest->ByteContent()->data(), latest->ByteContent()->size())
               << '\n' << "===================== end =====================";

        if(!proxy_used) {
            boost::shared_ptr<std::vector<SharedBuffer>> buffers(new std::vector<SharedBuffer>);
            buffers->push_back(latest->ByteContent());
            context->BridgedConnection()->PostAsyncWriteTask(buffers);
        } else {
            // TODO we need to decode the proxy message here, and then decide
            // whether we should write it to client
        }

        if(latest->type() == HttpObject::kHttpHeaders) {
            HttpHeaders *headers = reinterpret_cast<HttpHeaders*>(latest);
            std::string value;

            if(headers->find("Content-Length", value)
                    && boost::lexical_cast<std::size_t>(value) > 0) {
                context->connection()->PostAsyncReadTask();
                return kStop;
            }

            if(headers->find("Transfer-Encoding", value)
                      && value == "chunked") {
                context->connection()->PostAsyncReadTask();
                return kStop;
            }

            XDEBUG << context->connection()->identifier()
                   << " The response seems have no body, stop.";
        } else if(latest->type() == HttpObject::kHttpChunk) {
            HttpChunk *chunk = reinterpret_cast<HttpChunk*>(latest);
            if(!chunk->IsLast()) {
                context->connection()->PostAsyncReadTask();
                return kStop;
            }

            XDEBUG << context->connection()->identifier()
                   << " This is the last chunk of this response.";
        } else {
            XDEBUG << context->connection()->identifier()
                   << " The type of current response object is: " << latest->type()
                   << ", continue to read from server...";
            context->connection()->PostAsyncReadTask();
            return kStop;
        }

        /// The response is finished here, we should do cleanup work now
        HttpHeaders *headers = context->container()->RetrieveHeaders();
        std::string keep_alive;
        if(headers->find("Connection", keep_alive)) {
            std::for_each(keep_alive.begin(), keep_alive.end(),
                          [](char& c) { c = std::tolower(c); });
//            std::transform(connection.begin(), connection.end(),
//                           connection.begin(),
//                           std::ptr_fun<int, int>(std::tolower));
            if(keep_alive == "keep-alive") {
                XDEBUG << context->connection()->identifier()
                       << " The connection is a persistent connection.";
                context->connection()->PostCleanupTask(true);
            } else {
                XDEBUG << context->connection()->identifier()
                       << " Not a persistent connection, closing it...";
                context->connection()->PostCleanupTask(false);
            }
        }
        /// we always consider client connection persistent
        context->BridgedConnection()->PostCleanupTask(true);

        return kContinue;
    }

    virtual int priority() {
        /// response objects are processed one by one, and when response is not
        /// complete, the filtering process will stop here, so we set this
        /// filter's priority to middle, for those real emergency filters, we
        /// should put them before this, for those filters who need entire
        /// response, we can put them after this
        return kPriority;
    }

    virtual const std::string name() const {
        return "DefaultResponseFilter";
    }
};

#endif // BUILTIN_FILTERS_H
