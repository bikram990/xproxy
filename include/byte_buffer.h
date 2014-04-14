#ifndef BYTE_BUFFER_H
#define BYTE_BUFFER_H

#include <boost/asio.hpp>
#include "common.h"

/**
 * @brief The ByteBuffer class
 *
 * The ByteBuffer is a buffer for binary content.
 *
 * TODO: consider to use multiple memory blocks to improve performance, so there
 *       will be less copy operations
 */
class ByteBuffer {
public:
    typedef std::size_t size_type;
    typedef char* pointer_type;
    typedef const char* const_pointer_type;
    typedef char value_type;
    typedef char& reference_type;
    typedef const char& const_reference_type;
    typedef char* iterator;
    typedef const char* const_iterator;

    static const size_type npos = -1;

    enum { kDefaultSize = 1024, kGrowFactor = 2 }; // TODO proper value here

    virtual ~ByteBuffer() {
        if(data_) delete [] data_;
    }

    explicit ByteBuffer(size_type size = 0)
        : data_(new char[size ? size : kDefaultSize]), size_(0),
          capacity_(size ? size : kDefaultSize) {}

    ByteBuffer(const void *data, size_type size) : ByteBuffer(size) {
        // TODO verify data here: if(!data) blahblahblah...
        std::memcpy(data_, data, size);
        size_ = size;
    }

    ByteBuffer(const ByteBuffer& buffer) : ByteBuffer(buffer.data_, buffer.size_) {}

    ByteBuffer& operator=(const ByteBuffer& buffer) {
        if(this->capacity_ < buffer.size_) {
            delete [] data_;
            capacity_ = buffer.size_;
            data_ = new char[capacity_];
        }

        std::memcpy(data_, buffer.data_, buffer.size_);
        size_ = buffer.size_;

        return *this;
    }

    ByteBuffer& operator<<(char c) {
        EnsureSize(1);
        data_[size_++] = c;
        return *this;
    }

    ByteBuffer& operator<<(const std::string& str) {
        EnsureSize(str.size());
        for(auto it = str.begin(); it != str.end(); ++it) {
            data_[size_++] = *it;
        }
        return *this;
    }

    ByteBuffer& operator<<(const ByteBuffer& buffer) {
        EnsureSize(buffer.size());
        std::memcpy(data_ + size_, buffer.data(), buffer.size());
        size_ += buffer.size();
        return *this;
    }

    ByteBuffer& operator<<(const std::pair<const char*, size_type>& block) {
        EnsureSize(block.second);
        std::memcpy(data_ + size_, block.first, block.second);
        size_ += block.second;
        return *this;
    }

    ByteBuffer& operator<<(boost::asio::streambuf& stream) {
        operator<<(std::make_pair(boost::asio::buffer_cast<const char*>(stream.data()),
                                  stream.size()));
        stream.consume(stream.size());
        return *this;
    }

    pointer_type   	data()           { return data_; }
    const_pointer_type data()     const { return data_; }
    size_type      	size()     const { return size_; }
    bool           	empty()    const { return size_ == 0; }
    size_type      	capacity() const { return capacity_; }
    iterator       	begin()          { return data_; }
    const_iterator 	begin()    const { return data_; }
    iterator       	end()            { return data_ + size_; }
    const_iterator 	end()      const { return data_ + size_; }

    void reset() { size_ = 0; } // TODO we may do shrink job here

private:
    void EnsureSize(size_type size) {
        if(size_ + size <= capacity_)
            return;

        char *tmp = data_;
        // TODO the allocation algorithm here could be refined
        capacity_ = capacity_ * kGrowFactor + size;
        data_ = new char[capacity_];
        std::memcpy(data_, tmp, size_);
        delete [] tmp;
    }

    char *data_;
    size_type size_;
    size_type capacity_;
};

class SegmentalByteBuffer {
public:
    typedef std::vector::size_type segid_type;
    typedef std::vector::size_type seg_size_type;

    ByteBuffer::const_pointer_type invalid_pointer = nullptr;
    ByteBuffer::const_iterator invalid_iterator = nullptr;

    explicit SegmentalByteBuffer(size_type size = 0)
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
            return npos;

        Segment& seg = segments_[seg_id];
        auto orig_size = seg.end - seg.begin;
        auto new_end = seg.begin + size;
        auto delta_size = size - orig_size;

        if (delta_size > 0)
            buffer_->EnsureSize(delta_size);

        // if the new size != original size, we need to move the data after this segment
        if (delta_size != 0) {
            std::memmove(new_end, seg.end, buffer_->size() - seg.end);
            for (auto i = seg_id + 1; i < segments_.size(); ++i) {
                segments_[i].begin += delta_size;
                segments_[i].end   += delta_size;
            }
        }

        std::memcpy(seg.begin, data, size);
        return seg.begin;
    }

    ByteBuffer::size_type replace(segid_type seg_id, const ByteBuffer& buffer) {
        return replace(seg_id, buffer.data(), buffer.size());
    }

    SegmentalByteBuffer& append(const char *data, std::size_t size, bool new_seg = true) {
        *buffer_ << str;

        if (new_seg)
            segments_.push_back(std::forward(Segment{buffer_->size(), buffer_->size() + size}));
        else
            segments_.back().end += size;

        return *this;
    }

    SegmentalByteBuffer& append(const ByteBuffer& buffer, bool new_seg = true) {
        *buffer_ << str;

        if (new_seg)
            segments_.push_back(std::forward(Segment{buffer_->size(), buffer_->size() + buffer.size()}));
        else
            segments_.back().end += buffer.size();

        return *this;
    }

    SegmentalByteBuffer& append(const std::string& str, bool new_seg = true) {
        *buffer_ << str;

        if (new_seg)
            segments_.push_back(std::forward(Segment{buffer_->size(), buffer_->size() + str.length()}));
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
    seg_size_type SegmentCount() const { return segments_.size(); }
    bool empty() const { return buffer_->empty(); }

private:
    struct Segment {
        ByteBuffer::size_type begin;
        ByteBuffer::size_type end;
    };

    bool ValidateSegmentId(segid_type seg_id) {
        return seg_id >= 0
                && seg_id < segments_.size()
                && segments_[seg_id].begin >= current_pos_;
    }

private:
    ByteBuffer *buffer_;
    ByteBuffer::size_type current_pos_;
    std::vector<Segment> segments_;
};

typedef boost::shared_ptr<ByteBuffer> SharedByteBuffer;

#endif // BYTE_BUFFER_H
