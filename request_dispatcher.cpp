#include "log.h"
#include "request_dispatcher.h"

RequestDispatcher::RequestDispatcher() : service_() {
    TRACE_THIS_PTR;
}

RequestDispatcher::~RequestDispatcher() {
    TRACE_THIS_PTR;
}

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
            boost::array<char, 128> buf;
            boost::system::error_code error;

            std::size_t length = socket.read_some(boost::asio::buffer(buf), error);

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
    }
}
