#include <boost/thread.hpp>
#include <openssl/pem.h>
#include "resource_manager.h"

const std::string ResourceManager::ServerConfig::kConfPortKey("basic.port");
const std::string ResourceManager::ServerConfig::kConfGAEAppIdKey("proxy_gae.app_id");
const std::string ResourceManager::ServerConfig::kConfGAEDomainKey("proxy_gae.domain");

namespace {
    static boost::mutex mutex_;
    static std::auto_ptr<ResourceManager> manager_;
}

ResourceManager& ResourceManager::instance() {
    if(!manager_.get()) {
        boost::lock_guard<boost::mutex> s(mutex_);
        if(!manager_.get())
            manager_.reset(new ResourceManager);
    }
    return *manager_;
}

ResourceManager::ResourceManager()
    : server_config_(new ServerConfig()), rule_config_(new RuleConfig()) {
    TRACE_THIS_PTR;
}

ResourceManager::~ResourceManager() {
    TRACE_THIS_PTR;
}

bool ResourceManager::CertManager::LoadRootCA(const std::string& cert_file,
                                              const std::string& private_key_file) {
    FILE *fp = NULL;
    fp = std::fopen(cert_file.c_str(), "rb");
    if(!fp) {
        XERROR << "Failed to open Root CA certificate: " << cert_file;
        return false;
    }
    root_ca_.cert= PEM_read_X509(fp, NULL, NULL, NULL);
    if(!root_ca_.cert) {
        XERROR << "Failed to read Root CA certificate from file " << cert_file;
        std::fclose(fp);
        return false;
    }

    std::fclose(fp);
    fp = NULL;

    fp = std::fopen(private_key_file.c_str(), "rb");
    if(!fp) {
        XERROR << "Failed to open Root CA private key file: " << private_key_file;
        return false;
    }
    root_ca_.key= PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    if(!root_ca_.key) {
        XERROR << "Failed to read Root CA private key from file " << private_key_file;
        std::fclose(fp);
        return false;
    }

    std::fclose(fp);
    return true;
}

bool ResourceManager::CertManager::GenerateRootCA(CA& ca) {
    // TODO implement this
    return false;
}

bool ResourceManager::CertManager::GenerateCertificate(const std::string& host, CA& ca) {
    // TODO implement this
    return false;
}

bool ResourceManager::CertManager::SaveCertificate(const CA& ca,
                                                   const std::string& cert_file,
                                                   const std::string& private_key_file) {
    // TODO implement this
    return false;
}
