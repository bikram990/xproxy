#include "xproxy/conf/server_config.hpp"
#include "xproxy/proxy_server.hpp"
#include "xproxy/ssl/certificate_manager.hpp"

#ifdef WIN32
#include "openssl/applink.c"
#endif

int main(int, char**) {
    using namespace xproxy;
    using namespace xproxy::conf;
    using namespace xproxy::log;
    using namespace xproxy::ssl;

    initLogging();
    XINFO << "xProxy is starting...";

    if(!CertificateManager::init()) {
        XFATAL << "Unable to init certificate manager, exit.";
        return -1;
    }

    if (!ServerConfig::init()) {
        XFATAL << "Unable to init server configuration, exit.";
        return -1;
    }

//    ResourceManager::GetRuleConfig() << "youtube.com"
//                                     << "ytimg.com"
//                                     << "facebook.com"
//                                     << "google-analytics.com"
//                                     << "twitter.com";

    unsigned short port;
    if (!ServerConfig::instance().getConfig("basic.port", port))
        port = 7077;

    ProxyServer s(port);
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
