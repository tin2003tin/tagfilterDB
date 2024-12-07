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
