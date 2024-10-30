#pragma once

#include "tagfilterdb/common.hpp"
#include "tagfilterdb/support/murmurHash.hpp"

namespace tagfilterdb::compiler::atn {
class ATNDataView final {
  public:
    using value_type = int32_t;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = int32_t &;
    using const_reference = const int32_t &;
    using pointer = int32_t *;
    using const_pointer = const int32_t *;
    using iterator = const_pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    ATNDataView() = default;
    ATNDataView(const_pointer data, size_type size)
        : m_data(data), m_size(size) {}

    ATNDataView(const std::vector<int32_t> &serializedATN)
        : m_data(serializedATN.data()), m_size(serializedATN.size()) {}

    ATNDataView(const ATNDataView &) = default;

    ATNDataView &operator=(const ATNDataView &) = default;

    const_iterator begin() const { return m_data; }

    const_iterator cbegin() const { return m_data; }

    const_iterator end() const { return m_data + m_size; }

    const_iterator cend() const { return m_data + m_size; }

    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator(cbegin());
    }

    bool empty() const { return m_size == 0; }

    const_pointer data() const { return m_data; }

    size_type size() const { return m_size; }

    size_type size_bytes() const { return m_size * sizeof(value_type); }

    const_reference operator[](size_type index) const { return m_data[index]; }

  private:
    const_pointer m_data = nullptr;
    size_type m_size = 0;
};

inline bool operator==(const ATNDataView &lhs, const ATNDataView &rhs) {
    return (lhs.data() == rhs.data() && lhs.size() == rhs.size()) ||
           (lhs.size() == rhs.size() &&
            std::memcmp(lhs.data(), rhs.data(), lhs.size_bytes()) == 0);
}

inline bool operator!=(const ATNDataView &lhs, const ATNDataView &rhs) {
    return !operator==(lhs, rhs);
}

inline bool operator<(const ATNDataView &lhs, const ATNDataView &rhs) {
    int diff = std::memcmp(lhs.data(), rhs.data(),
                           std::min(lhs.size_bytes(), rhs.size_bytes()));
    return diff < 0 || (diff == 0 && lhs.size() < rhs.size());
}
} // namespace tagfilterdb::compiler::atn

namespace std {

using namespace tagfilterdb;
using namespace tagfilterdb::compiler::atn;

template <> struct hash<ATNDataView> {
    size_t operator()(const ATNDataView &r_atnDataView) const {
        return std::hash<std::string_view>{}(std::string_view(
            (const char *)r_atnDataView.data(), r_atnDataView.size()));
    };
};

} // namespace std