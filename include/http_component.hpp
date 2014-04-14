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

class HttpMessage {
public:
    enum FieldType {
        kRequestMethod, kRequestUri,
        kResponseStatus, kResponseMessage
    };

public:
    virtual ~HttpMessage() = default;

    // TODO maybe we don't need to provide this function
    void TurnOffRawBufSync() const {
        buf_sync_ = false;
    }

    /**
     * @brief TurnOnRawBufSync: turn on the buffer sync feature on this message.
     *
     * If this function is called, the raw buffer will be updated, and, every
     * change made to the message will casue its raw buffer updated, it will
     * affect performance.
     *
     * Recommended: make the sync feature off at first (it is off by default),
     * after all fields are set and will not change any more, then turn on it.
     *
     * A even better usage: make the sync feature off all the time, then set all
     * fields, and update the raw buffer manually at the same time, so we don't
     * need to turn on it to sync raw buffer. However, be careful when using in
     * this way.
     */
    void TurnOnRawBufSync() {
        buf_sync_ = true;
        UpdateRawBuffer();
    }

    int MajorVersion() const { return major_version_; }

    int MinorVersion() const { return minor_version_; }

    void MajorVersion(int version) {
        major_version_ = version;
        if (buf_sync_)
            UpdateFirstLine();
    }

    void MinorVersion(int version) {
        minor_version_ = version;
        if (buf_sync_)
            UpdateFirstLine();
    }

    // return reference of itself to support chaining invocation, same below
    HttpMessage& AddHeader(const std::string& name, const std::string& value) {
        headers_[name] = value;
        if (buf_sync_)
            UpdateHeaders();
        return *this;
    }

    bool FindHeader(const std::string& name, std::string& value) const {
        auto it = headers_.find(name);
        if (it == headers_.end())
            return false;
        value = it->second;
        return true;
    }

    HttpMessage& AppendBody(const char *data, std::size_t size, bool new_seg = true) {
        raw_buf_->append(data, size, new_seg);
        return *this;
    }

    HttpMessage& AppendBody(const std::string& str, bool new_seg = true) {
        raw_buf_->append(str, new_seg);
        return *this;
    }

    virtual std::string GetField(FieldType type) = 0;
    virtual void SetField(FieldType type, std::string&& value) = 0;

protected:
    HttpMessage()
        : major_version_(1),
          minor_version_(1),
          buf_sync_(false),
          raw_buf_(std::make_shared<SegmentalByteBuffer>(8192)) {} // TODO

    void UpdateRawBuffer() {
        UpdateFirstLine();
        UpdateHeaders();
    }

    virtual void UpdateFirstLine() = 0;

    void UpdateSegment(SegmentalByteBuffer::segid_type id, const ByteBuffer& buf) {
        if (raw_buf_->SegmentCount() > id) { // the raw buf contains this segment already
            auto ret = raw_buf_->replace(id, buf.data(), buf.size());
            if (ret == ByteBuffer::npos) {
                // TODO error handling here
            }
        } else if (raw_buf_->SegmentCount() == id) { // the raw buf is in correct state: ready for segment with id
            *raw_buf_ << buf;
        } else { // the raw buf is in incorrect state: need other segments to be inserted before this segment
            // TODO error handling here
        }
    }

protected:
    int major_version_;
    int minor_version_;
    std::map<std::string, std::string> headers_;

    bool buf_sync_; // whether we should sync raw buf or not while updating fields
    std::shared_ptr<SegmentalByteBuffer> raw_buf_;

private:
    void UpdateHeaders() {
        ByteBuffer temp(headers_.size() * 100); // the size
        for (auto it : headers_) {
            temp << it->first << ": " << it->second << CRLF;
        }
        temp << CRLF;

        UpdateSegment(1, temp);
    }

    DISABLE_COPY_AND_ASSIGNMENT(HttpMessage);
};

class HttpRequest : public HttpMessage {
public:
    HttpRequest() = default;
    virtual ~HttpRequest() = default;

    virtual std::string GetField(FieldType type) {
        switch (type) {
        case FieldType::kRequestMethod:
            return method_;
        case FieldType::kRequestUri:
            return uri_;
        case FieldType::kResponseStatus:
        case FieldType::kResponseMessage:
        default:
            ; // ignore
        }
    }

    virtual void SetField(FieldType type, std::string&& value) {
        switch (type) {
        case FieldType::kRequestMethod:
            method_ = std::forward(value);
            if (buf_sync_)
                UpdateFirstLine();
            break;
        case FieldType::kRequestUri:
            uri_ = std::forward(value);
            if (buf_sync_)
                UpdateFirstLine();
            break;
        case FieldType::kResponseStatus:
        case FieldType::kResponseMessage:
        default:
            ; // ignore
        }
    }

protected:
    virtual void UpdateFirstLine() {
        ByteBuffer temp(method_.length() + uri_.length() + 100);
        temp << method_ << ' ' << uri_  << ' ' << "HTTP/"
             << major_version_ << '.' << minor_version_ << CRLF;
        UpdateSegment(0, temp);
    }

private:
    std::string method_;
    std::string uri_;
};

class HttpResponse : public HttpMessage {
public:
    HttpResponse() = default;
    virtual ~HttpResponse() = default;

    virtual std::string GetField(FieldType type) {
        switch (type) {
        case FieldType::kResponseStatus:
            return std::to_string(status_);
        case FieldType::kResponseMessage:
            return message_;
        case FieldType::kRequestMethod:
        case FieldType::kRequestUri:
        default:
            ; // ignore
        }
    }

    virtual void SetField(FieldType type, std::string&& value) {
        switch (type) {
        case FieldType::kResponseStatus:
            status_ = std::stoi(value);
            if (buf_sync_)
                UpdateFirstLine();
            break;
        case FieldType::kResponseMessage:
            message_ = std::forward(message_);
            if (buf_sync_)
                UpdateFirstLine();
            break;
        case FieldType::kRequestMethod:
        case FieldType::kRequestUri:
        default:
            ; // ignore
        }
    }

protected:
    virtual void UpdateFirstLine() {
        ByteBuffer temp(message_.length() + 100);
        temp << "HTTP/" << major_version_ << '.' << minor_version_ << ' '
             << status_ << ' ' << message_ << CRLF;
        UpdateSegment(0, temp);
    }

private:
    int status_;
    std::string message_;
};

#endif // HTTP_COMPONENT_HPP
