#ifndef SEGMENTAL_BYTE_BUFFER_HPP
#define SEGMENTAL_BYTE_BUFFER_HPP

#include "memory/byte_buffer.hpp"

class SegmentalByteBuffer {
private:
    struct Segment {
        ByteBuffer::size_type begin;
        ByteBuffer::size_type end;
    };

public:
    typedef std::vector<Segment>::size_type segid_type;
    typedef std::vector<Segment>::size_type seg_size_type;

    ByteBuffer::const_pointer_type invalid_pointer = nullptr;
    ByteBuffer::const_iterator invalid_iterator = nullptr;

    explicit SegmentalByteBuffer(ByteBuffer::size_type size = 0)
        : buffer_(new ByteBuffer(size)),
          current_pos_(0) {}

    SegmentalByteBuffer(const SegmentalByteBuffer& buffer)
        : buffer_(new ByteBuffer(*buffer.buffer_)),
          current_pos_(buffer.current_pos_),
          segments_(buffer.segments_) {}

    SegmentalByteBuffer& operator=(const SegmentalByteBuffer& buffer) {
        *buffer_ = *buffer.buffer_;
        current_pos_ = buffer.current_pos_;
        segments_ = buffer.segments_;

        return *this;
    }

    virtual ~SegmentalByteBuffer() { if(buffer_) delete buffer_; }

    ByteBuffer::size_type replace(segid_type seg_id, ByteBuffer::const_pointer_type data, ByteBuffer::size_type size) {
        if (!ValidateSegmentId(seg_id))
            return ByteBuffer::npos;

        Segment& seg = segments_[seg_id];
        auto orig_size = seg.end - seg.begin;
        auto new_end = seg.begin + size;
        auto delta_size = size - orig_size;

        if (delta_size > 0)
            buffer_->EnsureSize(delta_size);

        // if the new size != original size, we need to move the data after this segment
        if (delta_size != 0) {
            std::memmove(buffer_->data() + new_end,
                         buffer_->data() + seg.end,
                         buffer_->size() - seg.end);
            for (auto i = seg_id + 1; i < segments_.size(); ++i) {
                segments_[i].begin += delta_size;
                segments_[i].end   += delta_size;
            }
        }

        std::memcpy(buffer_->data() + seg.begin, data, size);
        return seg.begin;
    }

    ByteBuffer::size_type replace(segid_type seg_id, const ByteBuffer& buffer) {
        return replace(seg_id, buffer.data(), buffer.size());
    }

    SegmentalByteBuffer& append(const char *data, std::size_t size, bool new_seg = true) {
        *buffer_ << ByteBuffer::wrap(data, size);

        if (new_seg)
            segments_.push_back(std::move(Segment{buffer_->size(), buffer_->size() + size}));
        else
            segments_.back().end += size;

        return *this;
    }

    SegmentalByteBuffer& append(const ByteBuffer& buffer, bool new_seg = true) {
        *buffer_ << buffer;

        if (new_seg)
            segments_.push_back(std::move(Segment{buffer_->size(), buffer_->size() + buffer.size()}));
        else
            segments_.back().end += buffer.size();

        return *this;
    }

    SegmentalByteBuffer& append(const std::string& str, bool new_seg = true) {
        *buffer_ << str;

        if (new_seg)
            segments_.push_back(std::move(Segment{buffer_->size(), buffer_->size() + str.length()}));
        else
            segments_.back().end += str.length();

        return *this;
    }

    void reset() {
        buffer_->reset();
        current_pos_ = 0;
        segments_.clear();
    }

    ByteBuffer::size_type size() const { return buffer_->size(); }

    ByteBuffer::size_type available() const {
        if (current_pos_ >= buffer_->size() || empty())
            return 0;
        return buffer_->size() - current_pos_;
    }

    ByteBuffer::const_pointer_type data() const {
        if (current_pos_ >= buffer_->size() || empty())
            return nullptr;
        return buffer_->data() + current_pos_;
    }

    seg_size_type SegmentCount() const { return segments_.size(); }

    bool empty() const { return buffer_->empty(); }

    void consume(ByteBuffer::size_type size) {
        current_pos_ += size;
        assert(current_pos_ < buffer_->size());
    }

private:
    bool ValidateSegmentId(segid_type seg_id) {
        return seg_id < segments_.size() && segments_[seg_id].begin >= current_pos_;
    }

private:
    ByteBuffer *buffer_;
    ByteBuffer::size_type current_pos_;
    std::vector<Segment> segments_;
};

#endif // SEGMENTAL_BYTE_BUFFER_HPP
