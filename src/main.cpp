#include "proxy_server.hpp"
#include "resource_manager.h"

#ifdef WIN32
#include "openssl/applink.c"
#endif

int main(int, char**) {
    xproxy::log::initLogging();
    XINFO << "xProxy is starting...";

    if(!ResourceManager::Init()) {
        XFATAL << "Failed to init resource, exit.";
        return -1;
    }

    ResourceManager::GetRuleConfig() << "youtube.com"
                                     << "ytimg.com"
                                     << "facebook.com"
                                     << "google-analytics.com"
                                     << "twitter.com";

    xproxy::ProxyServer s(ResourceManager::GetServerConfig().GetProxyPort());
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
