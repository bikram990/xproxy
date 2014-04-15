#include "http_message.hpp"

HttpMessage::HttpMessage()
    : major_version_(1),
      minor_version_(1),
      buf_sync_(false),
      raw_buf_(8192) {}

void HttpMessage::MajorVersion(int version) {
    major_version_ = version;
    if (buf_sync_)
        UpdateFirstLine();
}

void HttpMessage::MinorVersion(int version) {
    minor_version_ = version;
    if (buf_sync_)
        UpdateFirstLine();
}

HttpMessage &HttpMessage::AddHeader(const std::string &name, const std::string &value) {
    headers_[name] = value;
    if (buf_sync_)
        UpdateHeaders();
    return *this;
}

bool HttpMessage::FindHeader(const std::string &name, std::string &value) const {
    auto it = headers_.find(name);
    if (it == headers_.end())
        return false;
    value = it->second;
    return true;
}

HttpMessage &HttpMessage::AppendBody(const std::string &str, bool new_seg) {
    raw_buf_.append(str, new_seg);
    return *this;
}

HttpMessage &HttpMessage::AppendBody(const char *data, std::size_t size, bool new_seg) {
    raw_buf_.append(data, size, new_seg);
    return *this;
}

void HttpMessage::UpdateRawBuffer() {
    UpdateFirstLine();
    UpdateHeaders();
}

void HttpMessage::UpdateSegment(SegmentalByteBuffer::segid_type id, const ByteBuffer &buf) {
    if (raw_buf_.SegmentCount() > id) { // the raw buf contains this segment already
        auto ret = raw_buf_.replace(id, buf.data(), buf.size());
        if (ret == ByteBuffer::npos) {
            // TODO error handling here
        }
    } else if (raw_buf_.SegmentCount() == id) { // the raw buf is in correct state: ready for segment with id
        raw_buf_.append(buf);
    } else { // the raw buf is in incorrect state: need other segments to be inserted before this segment
        // TODO error handling here
    }
}

void HttpMessage::UpdateHeaders() {
    ByteBuffer temp(headers_.size() * 100); // the size
    for (auto it : headers_) {
        temp << it.first << ": " << it.second << CRLF;
    }
    temp << CRLF;

    UpdateSegment(1, temp);
}

void HttpMessage::reset() {
    major_version_ = 1;
    minor_version_ = 1;
    headers_.clear();
    buf_sync_ = false;
    raw_buf_.reset();
}
