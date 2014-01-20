#ifndef BUILTIN_FILTERS_H
#define BUILTIN_FILTERS_H

#include <boost/lexical_cast.hpp>
#include "filter.h"
#include "http_chunk.h"
#include "http_headers.h"
#include "http_request_initial.h"
#include "log.h"
#include "proxy_server.h"
#include "resource_manager.h"
#include "session_context.h"

class ProxyFilter : public Filter {
public:
    static const int kPriority = kHighest;

    ProxyFilter() : Filter(kBoth), is_proxied_(false) {}

    virtual FilterResult process(SessionContext& context, FilterType type) {
        if(type == Filter::kRequest) {
            std::string host;
            if(!context.request->RetrieveHeaders()->find("Host", host)) {
                // TODO add logic here
            }

            is_proxied_ = ResourceManager::GetRuleConfig().RequestProxy(host);
            if(!is_proxied_)
                return kContinue;

            // 1. reset the request
            context.request->reset();

            // 2. construct the proxy initial line
            HttpRequestInitial *initial = new HttpRequestInitial();
            initial->method().append("POST");
            initial->uri().append("/proxy");
            initial->SetMajorVersion(1);
            initial->SetMinorVersion(1);

            // 3. construct the proxy body
            temp_buffer_.consume(temp_buffer_.size());
            for(std::size_t i = 0; i < context.request->size(); ++i) {
                SharedBuffer buffer = context.request->RetrieveObject(i)->ByteContent();
                std::size_t copied = boost::asio::buffer_copy(temp_buffer_.prepare(buffer->size()),
                                                              boost::asio::buffer(buffer->data(),
                                                                                  buffer->size()));
                assert(copied == buffer->size());
                temp_buffer_.commit(copied);
            }
            HttpChunk *body = new HttpChunk();
            body->SetLast(true);
            *body << temp_buffer_;

            // 4. construct the proxy header
            HttpHeaders *headers = new HttpHeaders();
            headers->PushBack(HttpHeader("Host", ResourceManager::GetServerConfig().GetGAEAppId() + ".appspot.com"));
            headers->PushBack(HttpHeader("Connection", "keep-alive"));
            headers->PushBack(HttpHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:24.0) Gecko/20100101 Firefox/24.0"));
            headers->PushBack(HttpHeader("Content-Length", boost::lexical_cast<std::string>(body->ByteContent()->size())));
            if(context.https)
                headers->PushBack(HttpHeader("XProxy-Schema", "https://"));

            context.request->AppendObject(initial);
            context.request->AppendObject(headers);
            context.request->AppendObject(body);

            // we use https to connect to proxy server
            context.https = true;
            context.host = ResourceManager::GetServerConfig().GetGAEServerDomain();
            context.port = 443;
        } else if(type == Filter::kResponse) {
            if(!is_proxied_)
                return kContinue;

            if(context.response->RetrieveLatest()->type() == HttpObject::kHttpResponseInitial
                    || context.response->RetrieveLatest()->type() == HttpObject::kHttpHeaders) {
                // TODO use other erase method here is better
                context.response->reset();
            }
        } else {
            // do nothing here
        }

        return kContinue;
    }

    virtual int priority() {
        return kPriority;
    }

    virtual const std::string name() const {
        return "ProxyFilter";
    }

private:
    bool is_proxied_;
    boost::asio::streambuf temp_buffer_;
};

#endif // BUILTIN_FILTERS_H
