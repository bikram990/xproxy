#ifndef HTTP_COMPONENT_HPP
#define HTTP_COMPONENT_HPP

#include <memory>
#include "byte_buffer.h"
#include "common.h"

#define CRLF "\r\n"

class HttpComponent {
public:
    virtual ~HttpComponent() {}

    virtual std::shared_ptr<ByteBuffer> serialize() = 0;

    virtual void reset() = 0;
};

class HttpInitialLine : public HttpComponent {
public:
    virtual ~HttpInitialLine() = default;

    virtual std::shared_ptr<ByteBuffer> serialize() {
        if (modified_) {
            UpdateLine();
            modified_ = false;
        }
        return line_;
    }

    virtual void reset() {
        major_version_ = 0;
        minor_version_ = 0;
        modified_ = true;
        line_->reset();
    }

    int MajorVersion() const { return major_version_; }
    int MinorVersion() const { return minor_version_; }
    void MajorVersion(int version) { modified_ = true; major_version_ = version; }
    void MinorVersion(int version) { modified_ = true; minor_version_ = version; }

protected:
    HttpInitialLine()
        : modified_(true), line_(std::make_shared<ByteBuffer>()) {}

    virtual void UpdateLine() = 0;

protected:
    int major_version_;
    int minor_version_;

    bool modified_;
    std::shared_ptr<ByteBuffer> line_;

private:
    DISABLE_COPY_AND_ASSIGNMENT(HttpInitialLine);
};

class HttpRequestInitial : public HttpInitialLine {
public:
    DEFAULT_CTOR_AND_DTOR(HttpRequestInitial);

    std::string method() const { return method_; }
    std::string uri() const { return uri_; }
    void method(const std::string& method) { modified_ = true; method_ = method; }
    void uri(const std::string& uri) { modified_ = true; uri_ = uri; }

    virtual void reset() {
        HttpInitialLine::reset();

        method_.clear();
        uri_.clear();
    }

private:
    virtual void UpdateLine() {
        line_->reset();
        line_ << method_ << ' ' << uri_ << ' '
              << "HTTP/" << major_version_ << '.' << minor_version_ << CRLF;
    }

private:
    std::string method_;
    std::string uri_;
};

class HttpResponseInitial : public HttpInitialLine {
public:
    DEFAULT_CTOR_AND_DTOR(HttpResponseInitial);

    int status() const { return status_; }
    std::string message() const { return message_; }
    void status(int status) { modified_ = true; status_ = status; }
    void message(const std::string& message) { modified_ = true; message_ = message; }

    virtual void reset() {
        HttpInitialLine::reset();

        status_ = 0;
        message_.clear();
    }

private:
    virtual void UpdateLine() {
        line_->reset();
        line_ << "HTTP/" << major_version_ << '.' << minor_version_
              << ' ' << status_ << ' ' << message_ << CRLF;
    }

private:
    int status_;
    std::string message_;
};

class HttpHeaders : public HttpComponent {
public:
    HttpHeaders() : modified_(true), buffer_(std::make_shared<ByteBuffer>()) {}
    virtual ~HttpHeaders() = default;

    virtual std::shared_ptr<ByteBuffer> serialize() {
        if (modified_) {
            UpdateBuffer();
            modified_ = false;
        }
        return buffer_;
    }

    virtual void reset() {
        modified_ = true;
        headers_.clear();
        buffer_->reset();
    }

    bool find(const std::string& name, std::string& value) const {
        auto it = headers_.find(name);
        if (it == headers_.end())
            return false;
        value = it->second;
        return true;
    }

private:
    void UpdateBuffer() {
        buffer_->reset();
        for(auto it : headers_) {
            buffer_ << it->first << ": " << it->second << CRLF;
        }
        buffer_ << CRLF;
    }

private:
    bool modified_;
    std::map<std::string, std::string> headers_;
    std::shared_ptr<ByteBuffer> buffer_;
};

class HttpBody : public HttpComponent {
public:
    HttpBody() = default;
    virtual ~HttpBody() = default;

    virtual void reset() {
    }

    virtual std::shared_ptr<ByteBuffer> serialize() {
    }
};

#endif // HTTP_COMPONENT_HPP
