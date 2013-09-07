#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "log.h"

int main(int argc, char* argv[]) {
    Log::set_debug_level(boost::log::trivial::debug);
    Log::debug("xProxy is starting...");
    try {
        using namespace boost::asio;
        boost::asio::io_service io_service;
        boost::asio::signal_set sigs(io_service, SIGINT, SIGTERM);
        sigs.async_wait(boost::bind(&boost::asio::io_service::stop, &io_service));

        boost::asio::ip::tcp::resolver r(io_service);
        boost::asio::ip::tcp::resolver::query q("www.google.com", "http");
        boost::asio::ip::tcp::resolver::iterator it = r.resolve(q);
        boost::asio::ip::tcp::resolver::iterator end;
        for(; it != end; it++) {
            std::cout << (*it).endpoint().address() << std::endl;
        }
        //server s(io_service, 7077);
        //io_service.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    Log::debug("xProxy is stopped.");
    return 0;
}
