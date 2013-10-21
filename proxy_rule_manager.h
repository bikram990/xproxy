#ifndef PROXY_RULE_MANAGER_H
#define PROXY_RULE_MANAGER_H

#include <vector>
#include <boost/noncopyable.hpp>
#include "log.h"


class ProxyRuleManager : private boost::noncopyable {
public:
    static ProxyRuleManager& Instance() {
        return manager_;
    }

    ProxyRuleManager& operator<<(const std::string& domain) {
        domains_.push_back(domain);
        return *this;
    }

    bool RequestProxy(const std::string& host) {
        XINFO << "Checking if host " << host << " needs proxy...";
        for(std::vector<std::string>::iterator it = domains_.begin();
            it != domains_.end(); ++it) {
            XINFO << "Current rule is: " << *it;

            if(host.length() < it->length())
                continue;
            if(host.compare(host.length() - it->length(), it->length(), *it) == 0) {
                XINFO << "Host " << host << " matches rule " << *it << ", need proxy.";
                return true;
            }
        }

        XINFO << "Host " << host << " does not need proxy.";
        return false;
    }

private:
    ProxyRuleManager() {}
    ~ProxyRuleManager() {}

    std::vector<std::string> domains_;

    static ProxyRuleManager manager_;
};

#endif // PROXY_RULE_MANAGER_H
