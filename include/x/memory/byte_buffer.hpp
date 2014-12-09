#ifndef BYTE_BUFFER_HPP
#define BYTE_BUFFER_HPP

#include <cstring>
#include <cassert>
#include <memory>

namespace x {
namespace memory {

class byte_buffer {
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

    #warning proper value here
    enum { DEFAULT_SIZE = 1024, GROW_FACTOR = 2 };

    struct wrapper {
        const_pointer_type data;
        size_type size;
    };

    static wrapper wrap(const_pointer_type data, size_type size) {
        return wrapper{data, size};
    }

    virtual ~byte_buffer() {
        if(data_) delete [] data_;
    }

    explicit byte_buffer(size_type size = 0)
        : data_(new char[size ? size : DEFAULT_SIZE]), size_(0),
          capacity_(size ? size : DEFAULT_SIZE) {}

    byte_buffer(const void *data, size_type size) : byte_buffer(size) {
        assert(data);
        std::memcpy(data_, data, size);
        size_ = size;
    }

    byte_buffer(const byte_buffer& buffer) : byte_buffer(buffer.data_, buffer.size_) {}

    byte_buffer(byte_buffer&& buffer)
        : data_(buffer.data_), size_(buffer.size_), capacity_(buffer.capacity_) {
        buffer.data_ = nullptr;
        buffer.size_ = 0;
        buffer.capacity_ = 0;
    }

    byte_buffer& operator=(const byte_buffer& buffer) {
        if(this->capacity_ < buffer.size_) {
            delete [] data_;
            capacity_ = buffer.size_;
            data_ = new char[capacity_];
        }

        std::memcpy(data_, buffer.data_, buffer.size_);
        size_ = buffer.size_;

        return *this;
    }

    byte_buffer& operator=(byte_buffer&& buffer) {
        delete [] data_;

        data_ = buffer.data_;
        size_ = buffer.size_;
        capacity_ = buffer.capacity_;
        buffer.data_ = nullptr;
        buffer.size_ = 0;
        buffer.capacity_ = 0;

        return *this;
    }

    bool operator==(const byte_buffer& buffer) const {
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

    byte_buffer& operator<<(char c) {
        ensure_size(1);
        data_[size_++] = c;
        return *this;
    }

    byte_buffer& operator<<(const std::string& str) {
        ensure_size(str.size());
        for (auto it : str) {
            data_[size_++] = it;
        }
        return *this;
    }

    byte_buffer& operator<<(const char *cstr) {
        return operator<<(std::string(cstr));
    }

    byte_buffer& operator<<(int num) {
        return operator<<(std::to_string(num));
    }

    byte_buffer& operator<<(const byte_buffer& buffer) {
        ensure_size(buffer.size());
        std::memcpy(data_ + size_, buffer.data(), buffer.size());
        size_ += buffer.size();
        return *this;
    }

    byte_buffer& operator<<(const wrapper& w) {
        ensure_size(w.size);
        std::memcpy(data_ + size_, w.data, w.size);
        size_ += w.size;
        return *this;
    }

    template<class ContinuousByteSequence>
    byte_buffer& operator<<(const ContinuousByteSequence& sequence) {
        ensure_size(sequence.size());
        std::memcpy(data_ + size_, sequence.data(), sequence.size());
        size_ += sequence.size();
        return *this;
    }

    pointer_type       data()           { return data_; }
    const_pointer_type data()     const { return data_; }
    iterator           begin()          { return data_; }
    const_iterator     begin()    const { return data_; }
    iterator           end()            { return data_ + size_; }
    const_iterator     end()      const { return data_ + size_; }
    size_type          size()     const { return size_; }
    bool               empty()    const { return size_ == 0; }
    size_type          capacity() const { return capacity_; }
    #warning shrink job here?
    void               clear()          { size_ = 0; }

    pointer_type data(size_type pos) {
        return pos < size_ ? data_ + pos : nullptr;
    }

    const_pointer_type data(size_type pos) const {
        return pos < size_ ? data_ + pos : nullptr;
    }

    void erase(size_type first, size_type last) {
        if (first >= last) return;
        if (last > size_) return;

        if (last == size_) {
            size_ = first;
            return;
        }

        std::memmove(data_ + first, data_ + last, size_ - last);
        size_ -= (last - first);
    }

private:
    void ensure_size(size_type size) {
        if (size_ + size <= capacity_)
            return;

        char *tmp = data_;
        #warning the allocation algorithm here could be refined
        capacity_ = capacity_ * GROW_FACTOR + size;
        data_ = new char[capacity_];
        std::memcpy(data_, tmp, size_);
        delete [] tmp;
    }

    char *data_;
    size_type size_;
    size_type capacity_;
};

typedef std::shared_ptr<byte_buffer> buffer_ptr;

} // namespace memory
} // namespace x

#endif // BYTE_BUFFER_HPP
