#pragma once

#include "tagfilterdb/common.hpp"

namespace tagfilterdb::support {
constexpr size_t numericToSymbol(ssize_t value) {
    return static_cast<size_t>(value);
}
constexpr ssize_t symbolToNumeric(size_t value) {
    return static_cast<ssize_t>(value);
}

class Range final {
  public:
    static const Range INVALID;
    ssize_t start;
    ssize_t end;

    constexpr Range()
        : Range(static_cast<ssize_t>(-1), static_cast<ssize_t>(-2)) {}

    constexpr explicit Range(size_t start_, size_t end_)
        : Range(symbolToNumeric(start_), symbolToNumeric(end_)) {}

    constexpr Range(ssize_t start_, ssize_t end_) : start(start_), end(end_) {}

    constexpr size_t length() const {
        return end >= start ? static_cast<size_t>(end - start + 1) : 0;
    }

    constexpr bool operator==(const Range &other) const {
        return start == other.start && end == other.end;
    }

    size_t hashCode() const {
        size_t hash = 23;
        hash = hash * 31 + static_cast<size_t>(start);
        hash = hash * 31 + static_cast<size_t>(end);
        return hash;
    }

    bool startsBeforeDisjoint(const Range &other) const {
        return start < other.start && end < other.start;
    }

    bool startsBeforeNonDisjoint(const Range &other) const {
        return start <= other.start && end >= other.start;
    }

    bool startsAfter(const Range &other) const { return start > other.start; }

    bool startsAfterDisjoint(const Range &other) const {
        return start > other.end;
    }

    bool startsAfterNonDisjoint(const Range &other) const {
        return start > other.start && start <= other.end;
    }

    bool disjoint(const Range &other) const {
        return startsBeforeDisjoint(other) || startsAfterDisjoint(other);
    }

    bool adjacent(const Range &other) const {
        return start == other.end + 1 || end == other.start - 1;
    }

    bool properlyContains(const Range &other) const {
        return other.start >= start && other.end <= end;
    }

    Range merge(const Range &other) const {
        return Range(std::min(start, other.start), std::max(end, other.end));
    }

    Range overlap(const Range &other) const {
        return Range(std::max(start, other.start), std::min(end, other.end));
    }

    std::string toString() const {
        return std::to_string(start) + ".." + std::to_string(end);
    }
};
} // namespace tagfilterdb::support
