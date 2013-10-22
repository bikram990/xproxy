#include "proxy_server.h"
#include "log.h"
#include "resource_manager.h"

int main(int argc, char* argv[]) {
    xproxy::log::InitLogging();
    XINFO << "xProxy is starting...";

    if(!ResourceManager::instance().init()) {
        XFATAL << "Failed to init resource, exit.";
        return -1;
    }

    ResourceManager::instance().GetRuleConfig() << "youtube.com"
                                                << "facebook.com"
                                                << "twitter.com";

    try {
        ProxyServer s(ResourceManager::instance().GetServerConfig().GetProxyPort());
        s.Start();
    } catch (std::exception& e) {
        XERROR << "Exception occurred: " << e.what();
    }
    XINFO << "xProxy is stopped.";
    return 0;
}
