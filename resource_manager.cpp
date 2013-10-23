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
    X509 *x509 = NULL;
    x509 = X509_new();
    if(!x509) {
        XERROR << "Failed to create X509 structure.";
        return false;
    }

    // TODO should we always set the serial number to 1 ?
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

    // set the valid date, to 10 years
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 60 * 60 * 24 * 365 * 10);

    if(!GenerateKey(&ca.key)) {
        XERROR << "Failed to generate key for root CA.";
        X509_free(x509);
        return false;
    }
    X509_set_pubkey(x509, ca.key);

    X509_NAME *name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("xProxy Root CA"), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "OU", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("xProxy CA"),      -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, reinterpret_cast<const unsigned char *>("xProxy"),         -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "L",  MBSTRING_ASC, reinterpret_cast<const unsigned char *>("Lan"),            -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "ST", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("Internet"),       -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, reinterpret_cast<const unsigned char *>("CN"),             -1, -1, 0);

    X509_set_issuer_name(x509, name); //self signed

    if(!X509_sign(x509, ca.key, EVP_sha1())) { // sign cert using sha1
        XERROR << "Error signing root certificate.";
        X509_free(x509);
        EVP_PKEY_free(ca.key);
        return false;
    }

    ca.cert = x509;
    return true;
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

bool ResourceManager::CertManager::GenerateKey(EVP_PKEY **key) {
    EVP_PKEY *k = NULL;
    k = EVP_PKEY_new();
    if(!k) {
        XERROR << "Failed to create EVP_PKEY structure.";
        return false;
    }

    RSA *rsa = NULL;
    rsa = RSA_generate_key(2048, RSA_F4, NULL, NULL);
    if(!rsa) {
        XERROR << "Failed to generate 2048-bit RSA key.";
        EVP_PKEY_free(k);
        return false;
    }

    if(!EVP_PKEY_assign_RSA(k, rsa)) {
        XERROR << "Failed to assign rsa key to EVP_PKEY structure.";
        EVP_PKEY_free(k);
        RSA_free(rsa);
        return false;
    }

    *key = k;
    return true;
}
