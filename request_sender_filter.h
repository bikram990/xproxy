#ifndef REQUEST_SENDER_FILTER_H
#define REQUEST_SENDER_FILTER_H

#include "connection.h"
#include "filter.h"
#include "filter_context.h"
#include "http_object.h"

class RequestSenderFilter : public Filter {
public:
    RequestSenderFilter() : Filter(kRequest) {}

    virtual FilterResult process(FilterContext *context) {
        boost::shared_ptr<std::vector<boost::asio::const_buffer>> buffers(new std::vector<boost::asio::const_buffer>);
        HttpContainer *container = context->RequestContainer();
        for(std::size_t i = 0; i < container->size(); ++i) {
            HttpObject *object = container->RetrieveObject(i);
            buffers->push_back(boost::asio::buffer(object->ByteContent()->data(), object->ByteContent()->size()));
        }
        context->ServerConnection()->AsyncWrite(buffers);
        return Filter::kContinue;
    }

    virtual int priority() {
        return kMiddle;
    }

    virtual const std::string name() const {
        return "RequestSenderFilter";
    }
};

#endif // REQUEST_SENDER_FILTER_H
