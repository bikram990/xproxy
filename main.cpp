#include "proxy_server.h"
#include "resource_manager.h"

#ifdef WIN32
#include "openssl/applink.c"
#endif

int main(int argc, char* argv[]) {
    xproxy::log::InitLogging();
    XINFO << "xProxy is starting...";

    if(!ResourceManager::Init()) {
        XFATAL << "Failed to init resource, exit.";
        return -1;
    }

    xproxy::log::SetLogLevel(ResourceManager::GetServerConfig().GetLogLevel());

    ResourceManager::GetRuleConfig() << "youtube.com"
                                     << "ytimg.com"
                                     << "facebook.com"
                                     << "google-analytics.com"
                                     << "twitter.com";

    for(;;) {
        try {
            ProxyServer::Start();
            break;
        } catch(std::exception& e) {
            XERROR << "Exception occurred: " << e.what();
        }
    }

    XINFO << "xProxy is stopped.";
    return 0;
}
