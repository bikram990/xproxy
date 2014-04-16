#ifndef HTTP_MESSAGE_HPP
#define HTTP_MESSAGE_HPP

#include "common.h"
#include "memory/segmental_byte_buffer.hpp"

class HttpMessage {
public:
    enum FieldType {
        kRequestMethod, kRequestUri,
        kResponseStatus, kResponseMessage
    };

public:
    virtual ~HttpMessage() = default;

    // TODO maybe we don't need to provide this function
    void TurnOffRawBufSync() { buf_sync_ = false; }

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

    void MajorVersion(int version);

    void MinorVersion(int version);

    // return reference of itself to support chaining invocation, same below
    HttpMessage& AddHeader(const std::string& name, const std::string& value);

    bool FindHeader(const std::string& name, std::string& value) const;

    HttpMessage& AppendBody(const char *data, std::size_t size, bool new_seg = true);

    HttpMessage& AppendBody(const std::string& str, bool new_seg = true);

    virtual std::string GetField(FieldType type) const = 0;

    virtual void SetField(FieldType type, std::string&& value) = 0;

    virtual void reset();

    template<class Buffer, typename Copier>
    void serialize(Buffer& buffer, Copier&& copier) const {
        auto copied = copier(buffer, raw_buf_.data(), raw_buf_.available());
        raw_buf_.consume(copied);
    }

protected:
    HttpMessage(); // TODO

    void UpdateRawBuffer();

    virtual void UpdateFirstLine() = 0;

    void UpdateSegment(SegmentalByteBuffer::segid_type id, const ByteBuffer& buf);

protected:
    int major_version_;
    int minor_version_;
    std::map<std::string, std::string> headers_;

    bool buf_sync_; // whether we should sync raw buf or not while updating fields
    mutable SegmentalByteBuffer raw_buf_;

private:
    void UpdateHeaders();

    DISABLE_COPY_AND_ASSIGNMENT(HttpMessage);
};

#endif // HTTP_MESSAGE_HPP
