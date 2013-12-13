#ifndef REQUEST_SENDER_FILTER_H
#define REQUEST_SENDER_FILTER_H

#include "filter.h"
#include "filter_chain.h"
#include "http_headers.h"
#include "http_object.h"

class RequestSenderFilter : public Filter {
public:
    RequestSenderFilter() : Filter(kRequest) {}

    virtual FilterResult process() {
        std::vector<boost::asio::buffer> buffers;
        HttpContainer *container = chain_->RequestContainer();
        for(std::size_t i = 0; i < container->size(); ++i) {
            HttpObject *object = container->RetrieveObject(i);
            buffers->push_back(boost::asio::buffer(object->ByteContent()->data(), object->ByteContent()->size()));
        }
        chain_->ServerConnection()->AsyncWrite(buffers);
    }

    virtual int priority() {
        return kMiddle;
    }

    virtual const std::string name() const {
        return "RequestSenderFilter";
    }
};

#endif // REQUEST_SENDER_FILTER_H
