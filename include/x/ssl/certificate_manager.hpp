#ifndef CERTIFICATE_MANAGER_HPP
#define CERTIFICATE_MANAGER_HPP

#include <openssl/x509.h>

namespace x {
namespace ssl {

class certificate {
public:
    EVP_PKEY *key() const { return key_.get(); }
    X509 *certificate() const { return cert_.get(); }

    void set_key(EVP_PKEY *key) {
        key_.reset(key, ::EVP_PKEY_free);
    }

    void set_certificate(X509 *cert) {
        cert_.reset(cert, ::X509_free);
    }

private:
    std::shared_ptr<EVP_PKEY> key_;
    std::shared_ptr<X509> cert_;
};

class certificate_manager {
public:
    static certificate_manager& instance() {
        static certificate_manager manager;
        return manager;
    }

    static bool init() {
        auto& cm = instance();
        if (!cm.load_root_ca()) {
            if (!cm.generate_root_ca())
                return false;

            cm.save_root_ca();
        }

        if (!cm.load_dh_parameters()) {
            if (!cm.generate_dh_parameters())
                return false;

            cm.save_dh_parameters();
        }

        return true;
    }

    static certificate get_certificate(const std::string& host) {
        return instance().get_certificate(host);
    }

    static DH *get_dh_parameters() {
        return instance().get_dh_parameters();
    }

    DEFAULT_DTOR(certificate_manager);

private:
    certificate_manager() : cert_dir_("cert/"), dh_(nullptr, ::DH_free) {}

    bool load_root_ca(const std::string& file = "cert/xProxyRootCA.crt");
    bool save_root_ca(const std::string& file = "cert/xProxyRootCA.crt");
    bool generate_root_ca();

    certificate get_certificate(const std::string& host);
    bool load_certificate(const std::string& file, certificate& cert);
    bool save_certificate(const std::string& file, const certificate& cert);
    bool generate_certificate(const std::string& common_name, certificate& cert);

    DH *get_dh_parameters() const;
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
