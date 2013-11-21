#include "proxy_server.h"
#include "resource_manager.h"

int main(int argc, char* argv[]) {
    xproxy::log::InitLogging();
    XINFO << "xProxy is starting...";

    if(!ResourceManager::Init()) {
        XFATAL << "Failed to init resource, exit.";
        return -1;
    }

//    ResourceManager::instance().GetRuleConfig() << "youtube.com"
//                                                << "facebook.com"
//                                                << "google-analytics.com"
//                                                << "twitter.com";

    try {
        ProxyServer::Start();
    } catch (std::exception& e) {
        XERROR << "Exception occurred: " << e.what();
    }
    XINFO << "xProxy is stopped.";
    return 0;
}
