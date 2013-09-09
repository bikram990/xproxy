#include "log.h"
#include "http_proxy_server.h"

int main(int argc, char* argv[]) {
    Log::set_debug_level(boost::log::trivial::debug);
    Log::debug("xProxy is starting...");
    try {
        HttpProxyServer s;
        s.Start();
    } catch (std::exception& e) {
        Log::error("Exception occurred.");
        std::cerr << "Exception: " << e.what() << "\n";
    }
    Log::debug("xProxy is stopped.");
    return 0;
}
