#ifndef TAGFILTERDB_BITSET_H
#define  TAGFILTERDB_BITSET_H

#include <iostream>
#include <memory>

class Bitset {
public:
    char* data_;       // Pointer to the raw bitmap data_ (byte array)
    size_t size_;      // Number of bits

    Bitset() {
        data_ = nullptr;
    }

    ~Bitset() {
        delete []data_;
    }

    // Copy Constructor
    Bitset(const Bitset& other) : size_(other.size_) {
        if (other.data_) {
            size_t byteSize = (size_ + 7) / 8;
            data_ = new char[byteSize];
            std::memcpy(data_, other.data_, byteSize);
        } else {
            data_ = nullptr;
        }
    }

    // Move Constructor
    Bitset(Bitset&& other) noexcept : data_(other.data_), size_(other.size_) {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    // Copy Assignment Operator
    Bitset& operator=(const Bitset& other) {
        if (this == &other) return *this; // Handle self-assignment

        // Free existing data
        delete[] data_;

        size_ = other.size_;
        if (other.data_) {
            size_t byteSize = (size_ + 7) / 8;
            data_ = new char[byteSize];
            std::memcpy(data_, other.data_, byteSize);
        } else {
            data_ = nullptr;
        }

        return *this;
    }

    // Move Assignment Operator
    Bitset& operator=(Bitset&& other) noexcept {
        if (this == &other) return *this; // Handle self-assignment

        // Free existing data
        delete[] data_;

        // Move data
        data_ = other.data_;
        size_ = other.size_;

        // Nullify the source
        other.data_ = nullptr;
        other.size_ = 0;

        return *this;
    }

    void Setup(size_t bits) {
        size_ = bits;
        data_ = new char[(bits + 7) / 8]();
    }

    Bitset(size_t bits) : size_(bits) {
        data_ = new char[(bits + 7) / 8]();
    }

    void set(size_t index) {
        data_[index / 8] |= (1 << (index % 8));
    }

    void clear(size_t index) {
        data_[index / 8] &= ~(1 << (index % 8));
    }

    bool isSet(size_t index) const {
        return data_[index / 8] & (1 << (index % 8));
    }

    size_t count() const {
        size_t bitCount = 0;
        for (size_t i = 0; i < size_; ++i) {
            if (isSet(i)) {
                ++bitCount;
            }
        }
        return bitCount;
    }

    // Serialize the Bitset to a binary buffer
    void Serialize(char* buffer, size_t& offset) const {
        std::memcpy(buffer + offset, &size_, sizeof(size_));
        offset += sizeof(size_);

        size_t byteSize = (size_ + 7) / 8;
        std::memcpy(buffer + offset, data_, byteSize);
        offset += byteSize;
    }

    // Deserialize the Bitset from a binary buffer
    void Deserialize(const char* buffer, size_t& offset) {
        std::memcpy(&size_, buffer + offset, sizeof(size_));
        offset += sizeof(size_);

        delete[] data_;
        data_ = new char[(size_ + 7) / 8]();

        size_t byteSize = (size_ + 7) / 8;
        std::memcpy(data_, buffer + offset, byteSize);
        offset += byteSize;
    }

    std::string toString() const {
        std::string result;
        for (size_t i = 0; i < size_; ++i) {
            result += isSet(i) ? '1' : '0';
            if ((i + 1) % 8 == 0) {
                result += ' ';  // Add a space every 8 bits
            }
        }
        return result;
    }
};

#endif
