#include <iostream>
#include "log.h"
#include "http_proxy_server.h"

int main(int argc, char* argv[]) {
    xproxy::log::InitLogging();
    XINFO << "xProxy is starting...";

    try {
        HttpProxyServer s;
        s.Start();
    } catch (std::exception& e) {
        XERROR << "Exception occurred: " << e.what();
    }
    XINFO << "xProxy is stopped.";
    return 0;
}
