#include "proxy_server.h"
#include "log.h"
#include "proxy_configuration.h"
#include "proxy_rule_manager.h"

ProxyConfiguration g_config;

int main(int argc, char* argv[]) {
    xproxy::log::InitLogging();
    XINFO << "xProxy is starting...";

    if(!g_config.LoadConfig()) {
        XFATAL << "Failed to load config, exit.";
        return -1;
    }

    ProxyRuleManager::Instance() << "youtube.com"
                                 << "facebook.com"
                                 << "twitter.com";

    try {
        ProxyServer s(g_config.GetProxyPort());
        s.Start();
    } catch (std::exception& e) {
        XERROR << "Exception occurred: " << e.what();
    }
    XINFO << "xProxy is stopped.";
    return 0;
}
