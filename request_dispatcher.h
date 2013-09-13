#ifndef REQUEST_DISPATCHER_H
#define REQUEST_DISPATCHER_H

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include "log.h"


class RequestDispatcher : private boost::noncopyable {
public:
    RequestDispatcher();
    ~RequestDispatcher();

    template<typename BufT>
    bool DispatchRequest(std::string host, short port, BufT& buffer);

private:
    boost::asio::io_service service_;
};

template<typename BufT>
bool RequestDispatcher::DispatchRequest(std::string host, short port, BufT& buffer) {
    using namespace boost::asio::ip;
    try {
        tcp::resolver resolver(service_);
        tcp::resolver::query query(host);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        tcp::socket socket(service_);
        boost::asio::connect(socket, endpoint_iterator);

        for(;;) {
            //boost::array<char, 128> buf;
            boost::system::error_code error;

            //std::size_t length = socket.read_some(boost::asio::buffer(buf), error);
            boost::asio::read(socket, buffer, error);

            if (error == boost::asio::error::eof)
                break; // Connection closed cleanly by peer.
            else if (error)
                throw boost::system::system_error(error); // Some other error.

            //std::cout.write(buf.data(), length);
            // TODO uncomment this later when a suitable buffer is found
            //buffer << buf;
            //buffer.write(buf.data(), length);
        }
    }
    catch(std::exception& e) {
        XERROR << e.what();
        return false;
    }
    return true;
}

#endif // REQUEST_DISPATCHER_H
