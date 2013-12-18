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

class ServerConnectionObtainer : public Filter {
public:
    ServerConnectionObtainer() : Filter(kRequest) {}

    virtual FilterResult process(FilterContext *context) {
        XDEBUG << "In filter: " << name();

        HttpContainer *container = context->container();
        HttpHeaders *headers = container->RetrieveHeaders();

        if(!headers) {
            // TODO enhance the logic here
            XERROR << "No header object, this should never happen.";
            return kStop;
        }

        std::string host;
        short port = 80;
        if(!headers->find("Host", host)) {
            XERROR << "no host found, this should never happen.";
            return kStop;
        }

        std::string::size_type sep = host.find(':');
        if(sep != std::string::npos) {
            port = boost::lexical_cast<short>(host.substr(sep + 1));
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
        return kHighest - 1;
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
        return kHighest - 2;
    }

    virtual const std::string name() const {
        return "UriCanonicalizer";
    }
};

class RequestSenderFilter : public Filter {
public:
    RequestSenderFilter() : Filter(kRequest) {}

    virtual FilterResult process(FilterContext *context) {
        XDEBUG << "In filter: " << name();

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
        return kHighest - 3;
    }

    virtual const std::string name() const {
        return "RequestSenderFilter";
    }
};

class ResponseReceiverFilter : public Filter {
public:
    ResponseReceiverFilter() : Filter(kResponse) {}

    virtual FilterResult process(FilterContext *context) {
        XDEBUG << "In filter: " << name();

        HttpObject *latest = context->container()->RetrieveLatest();

        if(!latest) {
            XERROR << "Invalid object, this should never happen.";
            return kStop;
        }

        XDEBUG << "Dump http response object:\n"
               << "==================== begin ====================" << '\n'
               << std::string(latest->ByteContent()->data(), latest->ByteContent()->size())
               << '\n' << "===================== end =====================";

        boost::shared_ptr<std::vector<SharedBuffer>> buffers(new std::vector<SharedBuffer>);
        buffers->push_back(latest->ByteContent());
        context->BridgedConnection()->PostAsyncWriteTask(buffers);

        if(latest->type() != HttpObject::kHttpChunk) {
            context->connection()->PostAsyncReadTask();
            return kStop;
        }

        HttpChunk *chunk = reinterpret_cast<HttpChunk*>(latest);

        if(!chunk->IsLast()) {
            context->connection()->PostAsyncReadTask();
            return kStop;
        }

        return kContinue;
    }

    virtual int priority() {
        return kHighest;
    }

    virtual const std::string name() const {
        return "ResponseReceiverFilter";
    }
};

#endif // BUILTIN_FILTERS_H
