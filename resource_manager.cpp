#include <boost/date_time.hpp>
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
    std::string common_name = GetCommonName(host);
    std::map<std::string, CAPtr>::iterator it = ca_map_.find(common_name);
    if(it != ca_map_.end()) {
        return it->second;
    }

    XINFO << "Certificate for host " << host << " is not found, try loading from file...";
    CAPtr ca(boost::make_shared<CA>());
    std::string filename = GetCertificateFileName(common_name);

    if(LoadCertificate(filename, *ca)) {
        XINFO << "Certificate for host " << host << " is loaded from file.";
        ca_map_.insert(std::make_pair(common_name, ca));
        return ca;
    }

    XWARN << "Certificate for host " << host << " cannot be loaded from file, generating one...";
    if(!GenerateCertificate(common_name, *ca)) {
        XERROR << "Certificate for host " << host << " cannot be generated.";
        return CAPtr();
    }

    XINFO << "Certificate for host " << host << " is generated.";
    ca_map_.insert(std::make_pair(common_name, ca));
    SaveCertificate(filename, *ca);
    return ca;
}

ResourceManager::CertManager::DHParametersPtr ResourceManager::CertManager::GetDHParameters() {
    if(!dh_) {
        XERROR << "DH parameters is not initialized.";
        return DHParametersPtr();
    }
    return dh_;
}

bool ResourceManager::CertManager::LoadRootCA(const std::string& filename) {
    if(!root_ca_)
        root_ca_ = boost::make_shared<CA>();

    if(!LoadCertificate(filename, *root_ca_)) {
        XWARN << "Unable to load root CA.";
        return false;
    }

    XINFO << "Root CA loaded.";
    return true;
}

bool ResourceManager::CertManager::SaveRootCA(const std::string& filename) {
    if(!SaveCertificate(filename, *root_ca_)) {
        XWARN << "Unable to save root CA.";
        return false;
    }

    XINFO << "Root CA saved.";
    return true;
}

bool ResourceManager::CertManager::LoadCertificate(const std::string& filename, CA& ca) {
    FILE *fp = NULL;
    fp = std::fopen(filename.c_str(), "rb");
    if(!fp) {
        XWARN << "Unable to open file: " << filename;
        return false;
    }

    ca.cert = PEM_read_X509(fp, NULL, NULL, NULL);
    if(!ca.cert) {
        XERROR << "Failed to read certificate from file " << filename;
        std::fclose(fp);
        return false;
    }

    ca.key= PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    if(!ca.key) {
        XERROR << "Failed to read private key from file " << filename;
        std::fclose(fp);
        return false;
    }

    std::fclose(fp);
    XINFO << "Certificate and private key are loaded from " << filename;
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

bool ResourceManager::CertManager::GenerateCertificate(const std::string& common_name, CA& ca) {
    if(!root_ca_) {
        XERROR << "Root CA does not exist.";
        return false;
    }

    X509_REQ *req = NULL;
    EVP_PKEY *key = NULL;
    if(!GenerateRequest(common_name, &req, &key)) {
        XERROR << "Failed to Generate Certificate for common name " << common_name;
        return false;
    }
    if(X509_REQ_verify(req, key) != 1) {
        XERROR << "Failed to verify certificate request for common name " << common_name;
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

    // use exact time as cert's serial number to make it different, otherwise
    // browsers will complain that too many certs use the same serial number
    boost::posix_time::ptime time = boost::posix_time::microsec_clock::universal_time();
    boost::posix_time::ptime epoch = boost::posix_time::time_from_string("1970-01-01 00:00:00.000");
    ASN1_INTEGER_set(X509_get_serialNumber(cert), (time - epoch).total_microseconds());

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
    XINFO << "Certificate for common name " << common_name << " generated.";
    return true;
}

bool ResourceManager::CertManager::SaveCertificate(const std::string& filename, const CA& ca) {
    FILE *fp = NULL;
    fp = std::fopen(filename.c_str(), "wb");
    if(!fp) {
        XWARN << "Unable to open " << filename << " for writing.";
        return false;
    }

    if(!PEM_write_X509(fp, ca.cert)) {
        XERROR << "Failed to write certificate to " << filename;
        std::fclose(fp);
        return false;
    }

    if(!PEM_write_PrivateKey(fp, ca.key, NULL, NULL, 0, NULL, NULL)) {
        XERROR << "Failed to write private key to " << filename;
        std::fclose(fp);
        return false;
    }

    std::fclose(fp);
    XINFO << "Certificate and private key are saved to " << filename;
    return true;
}

bool ResourceManager::CertManager::LoadDHParameters(const std::string& filename) {
    if(!dh_)
        dh_ = boost::make_shared<DHParameters>();

    FILE *fp = NULL;
    fp = std::fopen(filename.c_str(), "rb");
    if(!fp) {
        XWARN << "Unable to open DH parameters file: " << filename;
        return false;
    }

    dh_->dh = PEM_read_DHparams(fp, NULL, NULL, NULL);
    if(!dh_->dh) {
        XERROR << "Failed to read DH parameters from file " << filename;
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

bool ResourceManager::CertManager::SaveDHParameters(const std::string& filename) {
    FILE *fp = NULL;
    fp = std::fopen(filename.c_str(), "wb");
    if(!fp) {
        XWARN << "Unable to open " << filename << " for writing.";
        return false;
    }

    if(!::PEM_write_DHparams(fp, dh_->dh)) {
        XERROR << "Failed to write DH parameters to " << filename;
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
    X509_NAME_add_entry_by_txt(name, "OU", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("xProxy Security"),   -1, -1, 0);
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

std::string ResourceManager::CertManager::GetCommonName(const std::string& host) {
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

std::string ResourceManager::CertManager::GetCertificateFileName(const std::string& common_name) {
    // TODO enhance this function
    std::string filename(common_name);
    if(filename[0] == '*')
        filename[0] = '^';

    filename += ".crt"; // add extension
    return filename;
}
