#include <iostream>
#include "log.h"
#include "http_proxy_server.h"

int main(int argc, char* argv[]) {
    xproxy::log::InitLogging();
    XDEBUG << "xProxy is starting...";

    try {
        HttpProxyServer s;
        s.Start();
    } catch (std::exception& e) {
        XERROR << "Exception occurred: " << e.what();
    }
    XDEBUG << "xProxy is stopped.";
    return 0;
}
