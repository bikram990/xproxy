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
    : server_config_(new ServerConfig()),
      rule_config_(new RuleConfig()),
      cert_manager_(new CertManager()) {
    TRACE_THIS_PTR;
}

ResourceManager::~ResourceManager() {
    TRACE_THIS_PTR;
}

ResourceManager::CertManager::CAPtr ResourceManager::CertManager::GetCertificate(const std::string& host) {
    std::map<std::string, CAPtr>::iterator it = ca_map_.find(host);
    if(it != ca_map_.end()) {
        return it->second;
    }

    XINFO << "Certificate for host " << host << " is not found, try loading from file...";
    CAPtr ca(boost::make_shared<CA>());
    if(LoadCertificate(host + ".crt", host + ".key", *ca)) {
        XINFO << "Certificate for host " << host << " is loaded from file.";
        ca_map_.insert(std::make_pair(host, ca));
        return ca;
    }

    XWARN << "Certificate for host " << host << " cannot be loaded from file, generating one...";
    if(!root_ca_) {
        XERROR << "Root CA is not initialized.";
        return CAPtr();
    }
    if(GenerateCertificate(host, *ca)) {
        XINFO << "Certificate for host " << host << " is generated.";
        ca_map_.insert(std::make_pair(host, ca));
        SaveCertificate(*ca, host + ".crt", host + ".key");
        return ca;
    }

    XERROR << "Certificate for host " << host << " cannot be generated.";
    return CAPtr();
}

bool ResourceManager::CertManager::LoadRootCA(const std::string& cert_file,
                                              const std::string& private_key_file) {
    if(!root_ca_)
        root_ca_ = boost::make_shared<CA>();

    if(!LoadCertificate(cert_file, private_key_file, *root_ca_)) {
        XERROR << "Failed to load root CA.";
        return false;
    }

    XINFO << "Root CA loaded.";
    return true;
}

bool ResourceManager::CertManager::SaveRootCA(const std::string& cert_file,
                                              const std::string& private_key_file) {
    if(!SaveCertificate(*root_ca_, cert_file, private_key_file)) {
        XERROR << "Failed to save root CA.";
        return false;
    }

    XINFO << "Root CA saved.";
    return true;
}

bool ResourceManager::CertManager::LoadCertificate(const std::string& cert_file,
                                                   const std::string& private_key_file,
                                                   CA& ca) {
    FILE *fp = NULL;
    fp = std::fopen(cert_file.c_str(), "rb");
    if(!fp) {
        XERROR << "Failed to open certificate: " << cert_file;
        return false;
    }
    ca.cert = PEM_read_X509(fp, NULL, NULL, NULL);
    if(!ca.cert) {
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
    ca.key= PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    if(!ca.key) {
        XERROR << "Failed to read Root CA private key from file " << private_key_file;
        std::fclose(fp);
        return false;
    }

    std::fclose(fp);
    XINFO << "Certificate " << cert_file << " loaded"
          << ", private key " << private_key_file << " loaded.";
    return true;
}

bool ResourceManager::CertManager::GenerateRootCA() {
    X509 *x509 = NULL;
    x509 = X509_new();
    if(!x509) {
        XERROR << "Failed to create X509 structure.";
        return false;
    }

    // we should set the serial number to 0 for root CA, otherwise it will not
    // be accepted by browsers
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 0);

    // set the valid date, to 10 years
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 60 * 60 * 24 * 365 * 10);

    if(!GenerateKey(&root_ca_->key)) {
        XERROR << "Failed to generate key for root CA.";
        X509_free(x509);
        return false;
    }
    X509_set_pubkey(x509, root_ca_->key);

    X509_NAME *name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("xProxy Root CA"), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "OU", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("xProxy CA"),      -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, reinterpret_cast<const unsigned char *>("xProxy"),         -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "L",  MBSTRING_ASC, reinterpret_cast<const unsigned char *>("Lan"),            -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "ST", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("Internet"),       -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, reinterpret_cast<const unsigned char *>("CN"),             -1, -1, 0);

    X509_set_issuer_name(x509, name); //self signed

    if(!X509_sign(x509, root_ca_->key, EVP_sha1())) { // sign cert using sha1
        XERROR << "Error signing root certificate.";
        X509_free(x509);
        EVP_PKEY_free(root_ca_->key);
        return false;
    }

    root_ca_->cert = x509;
    XINFO << "Root CA generated.";
    return true;
}

bool ResourceManager::CertManager::GenerateCertificate(const std::string& host, CA& ca) {
    if(!root_ca_) {
        XERROR << "Root CA does not exist.";
        return false;
    }

    X509_REQ *req = NULL;
    EVP_PKEY *key = NULL;
    if(!GenerateRequest(host, &req, &key)) { // TODO process the host here
        XERROR << "Failed to Generate Certificate for host " << host;
        return false;
    }
    if(X509_REQ_verify(req, key) != 1) {
        XERROR << "Failed to verify certificate request for host " << host;
        EVP_PKEY_free(key);
        X509_REQ_free(req);
        return false;
    }

    X509 *cert = NULL;
    cert = X509_new();
    if(!cert) {
        XERROR << "Failed to create X509 structure.";
        EVP_PKEY_free(key);
        X509_REQ_free(req);
        return false;
    }

    X509_NAME *name = X509_get_subject_name(root_ca_->cert);
    X509_set_issuer_name(cert, name);
    name = X509_REQ_get_subject_name(req);
    X509_set_subject_name(cert, name);
    X509_set_pubkey(cert, key);
    ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), 60 * 60 * 24 * 365 * 10);

    if(!X509_sign(cert, root_ca_->key, EVP_sha1())) {
        XERROR << "Error signing certificate.";
        EVP_PKEY_free(key);
        X509_REQ_free(req);
        X509_free(cert);
        return false;
    }

    ca.cert = cert;
    ca.key = key;
    X509_REQ_free(req);
    XINFO << "Certificate for host " << host << " generated.";
    return true;
}

bool ResourceManager::CertManager::SaveCertificate(const CA& ca,
                                                   const std::string& cert_file,
                                                   const std::string& private_key_file) {
    FILE *fp = NULL;

    fp = std::fopen(cert_file.c_str(), "wb");
    if(!fp) {
        XERROR << "Failed to open " << cert_file << " for writing.";
        return false;
    }
    if(!PEM_write_X509(fp, ca.cert)) {
        XERROR << "Failed to write certificate to " << cert_file;
        std::fclose(fp);
        return false;
    }

    std::fclose(fp);
    fp = NULL;

    fp = std::fopen(private_key_file.c_str(), "wb");
    if(!fp) {
        XERROR << "Failed to open " << cert_file << " for writing.";
        return false;
    }
    if(!PEM_write_PrivateKey(fp, ca.key, NULL, NULL, 0, NULL, NULL)) {
        XERROR << "Failed to write private key to " << private_key_file;
        std::fclose(fp);
        return false;
    }

    std::fclose(fp);
    XINFO << "Certificate saved to " << cert_file
          << ", private key saved to " << private_key_file;
    return true;
}

bool ResourceManager::CertManager::LoadDHParameters(const std::string& dh_file) {
    if(!dh_)
        dh_ = boost::make_shared<DHParameters>();

    FILE *fp = NULL;
    fp = std::fopen(dh_file.c_str(), "rb");
    if(!fp) {
        XERROR << "Failed to open DH parameters file: " << dh_file;
        return false;
    }
    dh_->dh = PEM_read_DHparams(fp, NULL, NULL, NULL);
    if(!dh_->dh) {
        XERROR << "Failed to read DH parameters from file " << dh_file;
        std::fclose(fp);
        return false;
    }

    std::fclose(fp);
    XINFO << "DH parameters loaded.";
    return true;
}

bool ResourceManager::CertManager::GenerateDHParameters() {
    dh_->dh = DH_generate_parameters(512, DH_GENERATOR_2, NULL, NULL); // 512 bits
    if(!dh_->dh) {
        XERROR << "Failed to generate DH parameters.";
        return false;
    }
    XINFO << "DH parameters generated.";
    return true;
}

bool ResourceManager::CertManager::SaveDHParameters(const std::string& dh_file) {
    FILE *fp = NULL;
    fp = std::fopen(dh_file.c_str(), "wb");
    if(!fp) {
        XERROR << "Failed to open " << dh_file << " for writing.";
        return false;
    }
    if(!::PEM_write_DHparams(fp, dh_->dh)) {
        XERROR << "Failed to write DH parameters to " << dh_file;
        std::fclose(fp);
        return false;
    }

    std::fclose(fp);
    XINFO << "DH parameters saved.";
    return true;
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

    if(!EVP_PKEY_assign_RSA(k, rsa)) { // now rsa's memory is managed by k
        XERROR << "Failed to assign rsa key to EVP_PKEY structure.";
        EVP_PKEY_free(k);
        RSA_free(rsa);
        return false;
    }

    *key = k; // we shouldn't free rsa here because it is managed by k
    return true;
}

bool ResourceManager::CertManager::GenerateRequest(const std::string& common_name,
                                                   X509_REQ **request, EVP_PKEY **key) {
    EVP_PKEY *k = NULL;
    if(!GenerateKey(&k)) {
        XERROR << "Failed to generate key.";
        return false;
    }

    X509_REQ *req = NULL;
    req = X509_REQ_new();
    if(!req) {
        XERROR << "Failed to create X509_REQ structure.";
        EVP_PKEY_free(k);
        return false;
    }
    X509_REQ_set_pubkey(req, k);

    X509_NAME *name = X509_REQ_get_subject_name(req);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char *>(common_name.c_str()), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "OU", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("xProxy Generated"),  -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, reinterpret_cast<const unsigned char *>(common_name.c_str()), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "L",  MBSTRING_ASC, reinterpret_cast<const unsigned char *>("Lan"),               -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "ST", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("Internet"),          -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, reinterpret_cast<const unsigned char *>("CN"),                -1, -1, 0);

    if(!X509_REQ_sign(req, k, EVP_sha1())) {
        XERROR << "Failed to sign request.";
        EVP_PKEY_free(k);
        X509_REQ_free(req);
        return false;
    }

    *request = req;
    *key = k;
    return true;
}
