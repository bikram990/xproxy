#include "x/conf/config.hpp"
#include "x/log/log.hpp"
#include "x/net/server.hpp"
#include "x/ssl/certificate_manager.hpp"

int main(int, char**) {
    using namespace x;
    using namespace x::conf;
    using namespace x::log;
    using namespace x::net;
    using namespace x::ssl;

    init_logging();
    XINFO << "xProxy is starting...";

//    ResourceManager::GetRuleConfig() << "youtube.com"
//                                     << "ytimg.com"
//                                     << "facebook.com"
//                                     << "google-analytics.com"
//                                     << "twitter.com";


    server s;
    if (!s.init()) {
        XFATAL << "failed to init server, exit.";
        return -1;
    }

    for(;;) {
        try {
            s.start();
            break;
        } catch(std::exception& e) {
            XERROR << "xProxy exception: " << e.what();
        }
    }

    XINFO << "xProxy is stopped.";
    return 0;
}
