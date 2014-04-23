#ifndef SEGMENTAL_BYTE_BUFFER_HPP
#define SEGMENTAL_BYTE_BUFFER_HPP

#include <vector>
#include <cassert>
#include "memory/byte_buffer.hpp"

class SegmentalByteBuffer {
public:
    typedef std::vector<ByteBuffer::size_type>::size_type seg_id_type;
    typedef std::vector<ByteBuffer::size_type>::size_type seg_size_type;

    // ByteBuffer::const_pointer_type invalid_pointer = nullptr;
    // ByteBuffer::const_iterator invalid_iterator = nullptr;

    explicit SegmentalByteBuffer(ByteBuffer::size_type size = 0)
        : buffer_(size),
          current_pos_(0) {
        segments_.push_back(0);
    }

    SegmentalByteBuffer(const SegmentalByteBuffer& buffer)
        : buffer_(buffer.buffer_),
          current_pos_(buffer.current_pos_),
          segments_(buffer.segments_) {}

    SegmentalByteBuffer& operator=(const SegmentalByteBuffer& buffer) {
        buffer_ = buffer.buffer_;
        current_pos_ = buffer.current_pos_;
        segments_ = buffer.segments_;

        return *this;
    }

    virtual ~SegmentalByteBuffer() = default;

    ByteBuffer::size_type replace(seg_id_type seg_id, ByteBuffer::const_pointer_type data, ByteBuffer::size_type size) {
        if (!ValidateSegmentId(seg_id))
            return ByteBuffer::npos;

        auto orig_size = segments_[seg_id] - segments_[seg_id - 1];
        auto delta_size = size - orig_size;

        if (delta_size > 0)
            buffer_.EnsureSize(delta_size);

        // if the new size != original size, we need to move the data after this segment
        if (delta_size != 0) {
            auto dest = buffer_.data() + segments_[seg_id] + delta_size;
            auto src = buffer_.data() + segments_[seg_id];
            auto sz = segments_[segments_.size() - 1] - segments_[seg_id];
            if (sz != 0) { // not the last segment
                std::memmove(dest, src, sz);
            }

            for (auto i = seg_id; i < segments_.size(); ++i) {
                segments_[i] += delta_size;
            }
        }

        std::memcpy(buffer_.data() + segments_[seg_id - 1], data, size);
        return segments_[seg_id - 1];
    }

    ByteBuffer::size_type replace(seg_id_type seg_id, const ByteBuffer& buffer) {
        return replace(seg_id, buffer.data(), buffer.size());
    }

    SegmentalByteBuffer& append(const void *data, std::size_t size, bool new_seg) {
        assert(buffer_.size() == segments_[segments_.size() - 1]);

        if (new_seg)
            segments_.push_back(buffer_.size() + size);
        else
            segments_.back() = segments_.back() + size;

        buffer_ << ByteBuffer::wrap(data, size);
        return *this;
    }

    SegmentalByteBuffer& append(const ByteBuffer& buffer, bool new_seg) {
        return append(buffer.data(), buffer.size(), new_seg);
    }

    SegmentalByteBuffer& append(const std::string& str, bool new_seg) {
        assert(buffer_.size() == segments_[segments_.size() - 1]);

        if (new_seg)
            segments_.push_back(buffer_.size() + str.length());
        else
            segments_.back() = segments_.back() + str.length();

        buffer_ << str;
        return *this;
    }

    template<typename Data>
    SegmentalByteBuffer& append(const Data& data, std::size_t size, bool new_seg) {
        assert(buffer_.size() == segments_[segments_.size() - 1]);

        if (new_seg)
            segments_.push_back(buffer_.size() + size);
        else
            segments_.back() = segments_.back() + size;

        buffer_ << data;
        return *this;
    }

    void reset() {
        buffer_.reset();
        current_pos_ = 0;
        segments_.clear();
    }

    ByteBuffer::size_type size() const { return buffer_.size(); }

    ByteBuffer::size_type available() const {
        if (current_pos_ >= buffer_.size() || empty())
            return 0;
        return buffer_.size() - current_pos_;
    }

    ByteBuffer::const_pointer_type data() const {
        if (current_pos_ >= buffer_.size() || empty())
            return nullptr;
        return buffer_.data() + current_pos_;
    }

    seg_size_type SegmentCount() const { return segments_.size() - 1; }

    bool empty() const { return buffer_.empty(); }

    void consume(ByteBuffer::size_type size) {
        assert(current_pos_ + size <= buffer_.size());
        current_pos_ += size;
    }

private:
    bool ValidateSegmentId(seg_id_type seg_id) {
        return seg_id >= 1
                && seg_id < segments_.size()
                && segments_[seg_id - 1] >= current_pos_;
    }

private:
    ByteBuffer buffer_;
    ByteBuffer::size_type current_pos_;
    std::vector<ByteBuffer::size_type> segments_; // stores the end of each segment
};

#endif // SEGMENTAL_BYTE_BUFFER_HPP
