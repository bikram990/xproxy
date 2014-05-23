#ifndef CERTIFICATE_MANAGER_HPP
#define CERTIFICATE_MANAGER_HPP

#include <openssl/x509.h>
#include "xproxy/util/singleton.hpp"

namespace xproxy {
namespace ssl {

class Certificate {
public:
    EVP_PKEY *key() const { return key_.get(); }
    X509 *certificate() const { return cert_.get(); }

    void setKey(EVP_PKEY *key) {
        key_.reset(key, ::EVP_PKEY_free);
    }

    void setCertificate(X509 *cert) {
        cert_.reset(cert, ::X509_free);
    }

private:
    std::shared_ptr<EVP_PKEY> key_;
    std::shared_ptr<X509> cert_;
};

class CertificateManager {
    friend class util::Singleton<CertificateManager>;
public:
    static bool init();
    static CertificateManager& instance();

    Certificate getCertificate(const std::string& host);
    DH *getDHParameters() const { return dh_.get(); }

    DEFAULT_DTOR(CertificateManager);

private:
    CertificateManager() : cert_dir_("cert/"), dh_(nullptr, ::DH_free) {}

    bool loadRootCA(const std::string& file = "cert/xProxyRootCA.crt");
    bool saveRootCA(const std::string& file = "cert/xProxyRootCA.crt");
    bool generateRootCA();

    bool loadCertificate(const std::string& file, Certificate& cert);
    bool saveCertificate(const std::string& file, const Certificate& cert);
    bool generateCertificate(const std::string& common_name, Certificate& cert);

    bool loadDHParameters(const std::string& file = "cert/dh.pem");
    bool saveDHParameters(const std::string& file = "cert/dh.pem");
    bool generateDHParameters();

    bool generateKey(EVP_PKEY **key);
    bool generateRequest(const std::string& common_name, X509_REQ **request, EVP_PKEY **key);

    std::string parseCommonName(const std::string& host);
    std::string getCertificateFileName(const std::string& common_name);

private:
    std::string cert_dir_;
    std::unique_ptr<DH, void(*)(DH*)> dh_;
    Certificate root_;
    std::map<std::string, Certificate> certificates_;

private:
    MAKE_NONCOPYABLE(CertificateManager);
};

} // namespace ssl
} // namespace xproxy

#endif // CERTIFICATE_MANAGER_HPP
