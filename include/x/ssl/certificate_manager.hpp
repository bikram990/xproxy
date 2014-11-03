#ifndef CERTIFICATE_MANAGER_HPP
#define CERTIFICATE_MANAGER_HPP

#include <openssl/x509.h>
#include "x/common.hpp"

namespace x {
namespace ssl {

class certificate {
public:
    EVP_PKEY *key() const { return key_.get(); }
    X509 *cert() const { return cert_.get(); }

    void set_key(EVP_PKEY *key) {
        key_.reset(key, ::EVP_PKEY_free);
    }

    void set_cert(X509 *cert) {
        cert_.reset(cert, ::X509_free);
    }

private:
    std::shared_ptr<EVP_PKEY> key_;
    std::shared_ptr<X509> cert_;
};

class certificate_manager {
public:
    certificate_manager() : cert_dir_("cert/"), dh_(nullptr, ::DH_free) {}

    DEFAULT_DTOR(certificate_manager);

    bool init() {
        if (!load_root_ca()) {
            if (!generate_root_ca())
                return false;

            save_root_ca();
        }

        if (!load_dh_parameters()) {
            if (!generate_dh_parameters())
                return false;

            save_dh_parameters();
        }

        return true;
    }

    certificate get_certificate(const std::string& host);
    DH *get_dh_parameters() const;

private:
    bool load_root_ca(const std::string& file = "cert/xProxyRootCA.crt");
    bool save_root_ca(const std::string& file = "cert/xProxyRootCA.crt");
    bool generate_root_ca();

    bool load_certificate(const std::string& file, certificate& cert);
    bool save_certificate(const std::string& file, const certificate& cert);
    bool generate_certificate(const std::string& common_name, certificate& cert);

    bool load_dh_parameters(const std::string& file = "cert/dh.pem");
    bool save_dh_parameters(const std::string& file = "cert/dh.pem");
    bool generate_dh_parameters();

    bool generate_key(EVP_PKEY **key);
    bool generate_request(const std::string& common_name, X509_REQ **request, EVP_PKEY **key);

    std::string parse_common_name(const std::string& host);
    std::string get_certificate_filename(const std::string& common_name);

private:
    std::string cert_dir_;
    std::unique_ptr<DH, void(*)(DH*)> dh_;
    certificate root_;
    std::map<std::string, certificate> certificates_;

    MAKE_NONCOPYABLE(certificate_manager);
};

} // namespace ssl
} // namespace x

#endif // CERTIFICATE_MANAGER_HPP
