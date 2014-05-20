#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <openssl/x509.h>
#include "log.h"
#include "util/singleton.hpp"

class ResourceManager : private boost::noncopyable {
    friend class xproxy::util::Singleton<ResourceManager>;
public:
    class ServerConfig;
    class RuleConfig;
    class CertManager;

    static ServerConfig& GetServerConfig() {
        return *instance().server_config_;
    }

    static RuleConfig& GetRuleConfig() {
        return *instance().rule_config_;
    }

    static CertManager& GetCertManager() {
        return *instance().cert_manager_;
    }

    static bool Init() {
        return instance().init();
    }

    ~ResourceManager();

private:
    ResourceManager();

    static ResourceManager& instance();

    bool init();

    std::unique_ptr<ServerConfig> server_config_;
    std::unique_ptr<RuleConfig> rule_config_;
    std::unique_ptr<CertManager> cert_manager_;
};

class ResourceManager::ServerConfig : private boost::noncopyable {
    friend class ResourceManager;
public:
    const static std::string kConfPortKey;
    const static std::string kConfThreadCountKey;
    const static std::string kConfLogLevel;
    const static std::string kConfGAEAppIdKey;
    const static std::string kConfGAEDomainKey;

    bool LoadConfig();
    bool LoadConfig(const std::string& conf_file);

    unsigned short GetProxyPort(unsigned short default_value = 7077);
    int GetThreadCount(int default_value = 5);
    std::string GetLogLevel(std::string default_value = "info");
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

    ~ServerConfig() {}

private:
    ServerConfig(const std::string& conf_file = "xproxy.conf")
        : conf_file_(conf_file) {}

    std::string conf_file_;
    boost::property_tree::ptree conf_tree_;
};

class ResourceManager::RuleConfig : private boost::noncopyable {
    friend class ResourceManager;
public:
    RuleConfig& operator<<(const std::string& domain);

    bool RequestProxy(const std::string& host);

    ~RuleConfig() {}

private:
    RuleConfig() {}

    std::vector<std::string> domains_;
};

class ResourceManager::CertManager : private boost::noncopyable {
    friend class ResourceManager;
public:
    struct CA : private boost::noncopyable {// to prevent multi memory free
        EVP_PKEY *key;
        X509 *cert;
        CA() : key(NULL), cert(NULL) {}
        ~CA() { if(key) ::EVP_PKEY_free(key); if(cert) ::X509_free(cert); }

        operator bool() { return key && cert; }
    };
    typedef boost::shared_ptr<CA> CAPtr;

    struct DHParameters : private boost::noncopyable {
        DH *dh;
        DHParameters() : dh(NULL) {}
        ~DHParameters() { if(dh) ::DH_free(dh); }

        operator bool() { return dh != NULL; }
    };
    typedef boost::shared_ptr<DHParameters> DHParametersPtr;

    CAPtr GetCertificate(const std::string& host);
    DHParametersPtr GetDHParameters();
    void SetCertificateDirectory(const std::string& directory);

    ~CertManager() {}

private:
    CertManager() : cert_dir_("cert/") {}

    bool LoadRootCA(const std::string& filename = "cert/xProxyRootCA.crt");
    bool SaveRootCA(const std::string& filename = "cert/xProxyRootCA.crt");
    bool LoadCertificate(const std::string& filename, CA& ca);
    bool SaveCertificate(const std::string& filename, const CA& ca);
    bool GenerateRootCA();
    bool GenerateCertificate(const std::string& common_name, CA& ca);
    bool LoadDHParameters(const std::string& filename = "cert/dh.pem");
    bool GenerateDHParameters();
    bool SaveDHParameters(const std::string& filename = "cert/dh.pem");
    bool GenerateKey(EVP_PKEY **key);
    bool GenerateRequest(const std::string& common_name, X509_REQ **request, EVP_PKEY **key);
    std::string GetCommonName(const std::string& host);
    std::string GetCertificateFileName(const std::string& common_name);

    std::string cert_dir_;
    CAPtr root_ca_;
    DHParametersPtr dh_;
    std::map<std::string, CAPtr> ca_map_;
};

inline bool ResourceManager::init() {
    bool s = server_config_->LoadConfig();

    bool cr = true;
    if(!cert_manager_->LoadRootCA()) {
        if(!cert_manager_->GenerateRootCA())
            cr = false;
        else
            cert_manager_->SaveRootCA();
    }

    bool cd = true;
    if(!cert_manager_->LoadDHParameters()) {
        if(!cert_manager_->GenerateDHParameters())
            cd = false;
        else
            cert_manager_->SaveDHParameters();
    }

    return s && cr && cd;
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

inline unsigned short ResourceManager::ServerConfig::GetProxyPort(unsigned short default_value) {
    unsigned short result;
    return GetConfig(kConfPortKey, result) ? result : default_value;
}

inline int ResourceManager::ServerConfig::GetThreadCount(int default_value) {
    int result;
    return GetConfig(kConfThreadCountKey, result) ? result : default_value;
}

inline std::string ResourceManager::ServerConfig::GetLogLevel(std::string default_value) {
    std::string result;
    return GetConfig(kConfLogLevel, result) ? result : default_value;
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
    for(std::vector<std::string>::iterator it = domains_.begin();
        it != domains_.end(); ++it) {
            if(host.length() < it->length())
                continue;
            if(host.compare(host.length() - it->length(), it->length(), *it) == 0) {
                XINFO << "Host " << host << " matches rule " << *it << ", need proxy.";
                return true;
            }
    }

    XDEBUG << "Host " << host << " does not need proxy.";
    return false;
}

inline ResourceManager::CertManager::DHParametersPtr ResourceManager::CertManager::GetDHParameters() {
    if(!dh_) {
        XERROR << "DH parameters is not initialized.";
        return DHParametersPtr();
    }
    return dh_;
}

inline void ResourceManager::CertManager::SetCertificateDirectory(const std::string& directory) {
    cert_dir_ = directory;
}

inline std::string ResourceManager::CertManager::GetCommonName(const std::string& host) {
    std::size_t dot_count = std::count(host.begin(), host.end(), '.');
    if(dot_count < 2) // means something like "something.com", or even something like "localhost"
        return host;

    std::string::size_type last = host.find_last_of('.');
    std::string::size_type penult = host.find_last_of('.', last - 1);
    if(last - penult <= 4) // means something like "something.com.cn"
        return host;

    std::string common_name(host);
    std::string::size_type first = host.find('.');
    common_name[first - 1] = '*';
    common_name.erase(0, first - 1);
    return common_name;
}

inline std::string ResourceManager::CertManager::GetCertificateFileName(const std::string& common_name) {
    // TODO enhance this function
    std::string filename(common_name);
    if(filename[0] == '*')
        filename[0] = '^';

    return cert_dir_ + filename + ".crt";
}

#endif // RESOURCE_MANAGER_H
