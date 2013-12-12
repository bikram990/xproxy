#ifndef BYTE_BUFFER_H
#define BYTE_BUFFER_H

#include <boost/asio.hpp>

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
    typedef char value_type;
    typedef char& reference_type;
    typedef char* iterator;
    typedef const char* const_iterator;

    static const size_type npos = -1;

    enum { kDefaultSize = 1024, kGrowFactor = 2 }; // TODO proper value here

    ~ByteBuffer() {
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

    ByteBuffer& operator<<(std::pair<const char*, size_type> block) {
        EnsureSize(block.second);
        std::memcpy(data_ + size_, block.first, block.second);
        return *this;
    }

    ByteBuffer& operator<<(boost::asio::streambuf& stream) {
        operator<<(std::make_pair(boost::asio::buffer_cast<const char*>(stream.data()),
                                  stream.size()));
        stream.consume(stream.size());
        return *this;
    }

    pointer_type data() { return data_; }

    size_type size() { return size_; }

    size_type capacity() { return capacity_; }

    void reset() { size_ = 0; }


private:
    void EnsureSize(size_type size) {
        if(size_ + size <= capacity_)
            return;

        char *tmp = data_;
        capacity_ *= kGrowFactor;
        data_ = new char[capacity_];
        std::memcpy(data_, tmp, size_);
        delete [] tmp;
    }

    char *data_;
    size_type size_;
    size_type capacity_;
};

typedef boost::shared_ptr<ByteBuffer> SharedBuffer;

/**
 * @brief The SharedBufferSequence class
 *
 * A helper class
 */
class SharedBufferSequence {
public:
    void reset() {
        for(auto it = buffers_->begin(); it != buffers_->end(); ++it) {
            it->reset();
        }
    }

private:
    boost::shared_ptr<std::list<SharedBuffer>> buffers_;
};

#endif // BYTE_BUFFER_H
