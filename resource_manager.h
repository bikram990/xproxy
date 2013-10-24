#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <map>
#include <boost/noncopyable.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <openssl/x509.h>
#include "log.h"

class ResourceManager : private boost::noncopyable {
    friend class std::auto_ptr<ResourceManager>;
public:
    class ServerConfig;
    class RuleConfig;
    class CertManager;

    static ResourceManager& instance();

    ServerConfig& GetServerConfig();
    RuleConfig& GetRuleConfig();
    CertManager& GetCertManager();

    bool init();

private:
    ResourceManager();
    ~ResourceManager();

    std::auto_ptr<ServerConfig> server_config_;
    std::auto_ptr<RuleConfig> rule_config_;
    std::auto_ptr<CertManager> cert_manager_;
};

class ResourceManager::ServerConfig : private boost::noncopyable {
    friend class ResourceManager;
    friend class std::auto_ptr<ServerConfig>;
public:
    const static std::string kConfPortKey;
    const static std::string kConfGAEAppIdKey;
    const static std::string kConfGAEDomainKey;

    bool LoadConfig();
    bool LoadConfig(const std::string& conf_file);

    short GetProxyPort(short default_value = 7077);
    const std::string GetGAEAppId();
    const std::string GetGAEServerDomain(const std::string& default_value = "google.com.hk");

    template<typename TypeT>
    bool GetConfig(const std::string& path, TypeT& return_value) {
        try {
            return_value = conf_tree_.get<TypeT>(path);
        } catch(boost::property_tree::ptree_error& e) {
            XWARN << "Failed to read config, path: " << path
                  << ", reason: " << e.what();
            return false;
        }
        return true;
    }

private:
    ServerConfig(const std::string& conf_file = "xproxy.conf")
        : conf_file_(conf_file) {}
    ~ServerConfig() {}

    std::string conf_file_;
    boost::property_tree::ptree conf_tree_;
};

class ResourceManager::RuleConfig : private boost::noncopyable {
    friend class ResourceManager;
    friend class std::auto_ptr<RuleConfig>;
public:
    RuleConfig& operator<<(const std::string& domain);

    bool RequestProxy(const std::string& host);

private:
    RuleConfig() {}
    ~RuleConfig() {}

    std::vector<std::string> domains_;
};

class ResourceManager::CertManager : private boost::noncopyable {
    friend class ResourceManager;
    friend class std::auto_ptr<CertManager>;
public:
    struct CA {
        EVP_PKEY *key;
        X509 *cert;
        CA() : key(NULL), cert(NULL) {}
        ~CA() { if(key) ::EVP_PKEY_free(key); if(cert) ::X509_free(cert); }

        operator bool() { return key && cert; }
    };

private:
    CertManager() {}
    ~CertManager() {}

    bool LoadRootCA(const std::string& cert_file = "xProxyRootCA.crt",
                    const std::string& private_key_file = "xProxyRootCA.key");
    bool SaveRootCA(const std::string& cert_file = "xProxyRootCA.crt",
                    const std::string& private_key_file = "xProxyRootCA.key");
    bool LoadCertificate(const std::string& cert_file,
                         const std::string& private_key_file,
                         CA& ca);
    bool SaveCertificate(const CA& ca,
                         const std::string& cert_file,
                         const std::string& private_key_file);
    bool GenerateRootCA();
    bool GenerateCertificate(const std::string& host, CA& ca);
    bool GenerateKey(EVP_PKEY **key);
    bool GenerateRequest(const std::string& common_name, X509_REQ **request, EVP_PKEY **key);

    CA root_ca_;
    std::map<std::string, CA> ca_map_;
};

inline ResourceManager::ServerConfig& ResourceManager::GetServerConfig() {
    return *server_config_;
}

inline ResourceManager::RuleConfig& ResourceManager::GetRuleConfig() {
    return *rule_config_;
}

inline ResourceManager::CertManager& ResourceManager::GetCertManager() {
    return *cert_manager_;
}

inline bool ResourceManager::init() {
    return server_config_->LoadConfig();
}

inline bool ResourceManager::ServerConfig::LoadConfig() {
    try {
        boost::property_tree::ini_parser::read_ini(conf_file_, conf_tree_);
    } catch(boost::property_tree::ini_parser::ini_parser_error& e) {
        XFATAL << "Failed to load configuration from config file: " << conf_file_
               << ", reason: " << e.what();
        return false;
    }
    return true;
}

inline bool ResourceManager::ServerConfig::LoadConfig(const std::string& conf_file) {
    if(conf_file_ != conf_file)
        conf_file_ = conf_file;
    return LoadConfig();
}

inline short ResourceManager::ServerConfig::GetProxyPort(short default_value) {
    short result;
    return GetConfig(kConfPortKey, result) ? result : default_value;
}

inline const std::string ResourceManager::ServerConfig::GetGAEAppId() {
    std::string result;
    return GetConfig(kConfGAEAppIdKey, result) ? result : "";
}

inline const std::string ResourceManager::ServerConfig::GetGAEServerDomain(const std::string& default_value) {
    std::string result;
    return GetConfig(kConfGAEDomainKey, result) ? result : default_value;
}

inline ResourceManager::RuleConfig& ResourceManager::RuleConfig::operator<<(const std::string& domain) {
    domains_.push_back(domain);
    return *this;
}

inline bool ResourceManager::RuleConfig::RequestProxy(const std::string& host) {
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

#endif // RESOURCE_MANAGER_H
