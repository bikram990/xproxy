#ifndef BYTE_BUFFER_HPP
#define BYTE_BUFFER_HPP

#include <string>

/**
 * @brief The ByteBuffer class
 *
 * The ByteBuffer is a buffer for binary content.
 *
 */
class ByteBuffer {
public:
    friend class SegmentalByteBuffer;

    typedef std::size_t size_type;
    typedef char* pointer_type;
    typedef const char* const_pointer_type;
    typedef char value_type;
    typedef char& reference_type;
    typedef const char& const_reference_type;
    typedef char* iterator;
    typedef const char* const_iterator;

    static const size_type npos = -1;

    // TODO proper value here
    enum { kDefaultSize = 1024, kGrowFactor = 2 };

    struct wrapper {
        const_pointer_type d;
        size_type s;

        const_pointer_type data() const { return d; }
        size_type size() const { return s; }
    };

    static wrapper wrap(const_pointer_type data, size_type size) {
        return wrapper{data, size};
    }

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

    bool operator==(const ByteBuffer& buffer) const {
        if (this == &buffer)
            return true;

        if (size_ != buffer.size_)
            return false;

        for (auto i = 0; i < size_; ++i) {
            if (*(data_ + i) != *(buffer.data_ + i))
                return false;
        }

        return true;
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

    ByteBuffer& operator<<(const_pointer_type data) {
        return operator<<(std::string{data});
    }

    ByteBuffer& operator<<(int num) {
        return operator<<(std::to_string(num));
    }

    template<class Buffer>
    ByteBuffer& operator<<(const Buffer& buffer) {
        EnsureSize(buffer.size());
        std::memcpy(data_ + size_, buffer.data(), buffer.size());
        size_ += buffer.size();
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

    void clear() { size_ = 0; }
    void reset() { size_ = 0; } // TODO we may do shrink job here

    pointer_type data(size_type pos) {
        return pos < size_ ? data_ + pos : nullptr;
    }

    const_pointer_type data(size_type pos) const {
        return pos < size_ ? data_ + pos : nullptr;
    }

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

#endif // BYTE_BUFFER_HPP
