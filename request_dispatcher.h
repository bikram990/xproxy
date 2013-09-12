#ifndef REQUEST_DISPATCHER_H
#define REQUEST_DISPATCHER_H

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>


class RequestDispatcher : private boost::noncopyable {
public:
    RequestDispatcher();
    ~RequestDispatcher();

    template<typename BufT>
    bool DispatchRequest(std::string host, short port, BufT& buffer);

private:
    boost::asio::io_service service_;
};

#endif // REQUEST_DISPATCHER_H
