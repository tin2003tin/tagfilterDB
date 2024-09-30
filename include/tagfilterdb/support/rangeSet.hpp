#pragma once

#include "range.hpp"
#include "tagfilterdb/exceptions.hpp"

namespace tagfilterdb::support {
class RangeSet final {
  public:
    static RangeSet const COMPLETE_CHAR_SET;
    static RangeSet const EMPTY_SET;

  private:
    std::vector<Range> _ranges;
    explicit RangeSet(std::vector<Range> &&ranges) {}

  public:
    RangeSet() {}
    RangeSet(RangeSet const &set) { _ranges = set._ranges; }
    RangeSet(RangeSet &&set) : RangeSet(std::move(set._ranges)) {}

    template <typename T1, typename... T_NEXT>
    RangeSet(int, T1 t1, T_NEXT &&...next) : RangeSet() {}

    RangeSet &operator=(RangeSet const &other) {
        _ranges = other._ranges;
        return *this;
    }
    RangeSet operator=(RangeSet &&other) {
        _ranges = std::move(other._ranges);
        return *this;
    }

    static RangeSet of(ssize_t a) { return RangeSet({Range(a, a)}); }
    static RangeSet of(ssize_t a, ssize_t b) { return RangeSet({Range(a, b)}); }

    void clear() { _ranges.clear(); }

    void add(ssize_t el) { add(el, el); }

    void add(ssize_t start, ssize_t end) { add(Range(start, end)); }

    void add(const Range &addition) {
        if (addition.end < addition.start) {
            return;
        }
        for (auto iter = _ranges.begin(); iter != _ranges.end(); iter++) {
            Range r = *iter;
            if (addition == r) {
                return;
            }

            if (addition.adjacent(r) || !addition.disjoint(r)) {
                Range bigger = addition.merge(r);
                *iter = bigger;
                while (iter + 1 != _ranges.end()) {
                    Range next = *++iter;
                    if (!bigger.adjacent(next) && bigger.disjoint(next)) {
                        break;
                    }
                    iter = _ranges.erase(iter);
                    --iter;
                    *iter = bigger.merge(next);
                }
                return;
            }
            if (addition.startsBeforeDisjoint(r)) {
                _ranges.insert(iter, addition);
                return;
            }
        }
        _ranges.push_back(addition);
    }

    RangeSet addAll(const RangeSet &set) {
        for (auto const &range : set._ranges) {
            add(range);
        }
        return *this;
    }

    template <typename T1, typename... T_NEXT>
    void addItems(T1 t1, T_NEXT &&...next) {
        add(t1);
        addItems(std::forward<T_NEXT>(next)...);
    }

    RangeSet complement(ssize_t minElement, ssize_t maxElement) const {
        return complement(RangeSet::of(minElement, maxElement));
    }

    RangeSet complement(const RangeSet &vocabulary) const {
        return vocabulary.subtract(*this);
    }

    // TODO: (RangeSet) subtract
    RangeSet subtract(const RangeSet &other) const {
        return subtract(*this, other);
    }

    static RangeSet subtract(const RangeSet &left, const RangeSet &right) {
        if (left.empty()) {
            return RangeSet();
        }

        if (right.empty()) {
            // right set has no elements; just return the copy of the current
            // set
            return left;
        }

        RangeSet result(left);
        size_t resultI = 0;
        size_t rightI = 0;
        while (resultI < result._ranges.size() &&
               rightI < right._ranges.size()) {
            Range &resultInterval = result._ranges[resultI];
            const Range &rightInterval = right._ranges[rightI];

            // operation: (resultInterval - rightInterval) and update indexes

            if (rightInterval.end < resultInterval.start) {
                rightI++;
                continue;
            }

            if (rightInterval.start > resultInterval.end) {
                resultI++;
                continue;
            }

            Range beforeCurrent;
            Range afterCurrent;
            if (rightInterval.start > resultInterval.start) {
                beforeCurrent =
                    Range(resultInterval.start, rightInterval.start - 1);
            }

            if (rightInterval.end < resultInterval.end) {
                afterCurrent = Range(rightInterval.end + 1, resultInterval.end);
            }

            if (beforeCurrent.start > -1) { // -1 is the default value
                if (afterCurrent.start > -1) {
                    // split the current interval into two
                    result._ranges[resultI] = beforeCurrent;
                    result._ranges.insert(result._ranges.begin() + resultI + 1,
                                          afterCurrent);
                    resultI++;
                    rightI++;
                } else {
                    // replace the current interval
                    result._ranges[resultI] = beforeCurrent;
                    resultI++;
                }
            } else {
                if (afterCurrent.start > -1) {
                    // replace the current interval
                    result._ranges[resultI] = afterCurrent;
                    rightI++;
                } else {
                    // remove the current interval (thus no need to increment
                    // resultI)
                    result._ranges.erase(result._ranges.begin() + resultI);
                }
            }
        }
        return result;
    }

    static RangeSet Or(const std::vector<RangeSet> &sets) {
        RangeSet result;
        for (const auto &s : sets) {
            result.addAll(s);
        }
        return result;
    }

    RangeSet Or(const RangeSet &a) const {
        RangeSet result;
        result.addAll(*this);
        result.addAll(a);
        return result;
    }

    RangeSet And(const RangeSet &other) const {
        RangeSet intersection;
        size_t i = 0;
        size_t j = 0;

        // iterate down both interval lists looking for nondisjoint intervals
        while (i < _ranges.size() && j < other._ranges.size()) {
            Range mine = _ranges[i];
            Range theirs = other._ranges[j];

            if (mine.startsBeforeDisjoint(theirs)) {
                // move this iterator looking for interval that might overlap
                i++;
            } else if (theirs.startsBeforeDisjoint(mine)) {
                // move other iterator looking for interval that might overlap
                j++;
            } else if (mine.properlyContains(theirs)) {
                // overlap, add intersection, get next theirs
                intersection.add(mine.overlap(theirs));
                j++;
            } else if (theirs.properlyContains(mine)) {
                // overlap, add intersection, get next mine
                intersection.add(mine.overlap(theirs));
                i++;
            } else if (!mine.disjoint(theirs)) {
                // overlap, add intersection
                intersection.add(mine.overlap(theirs));

                // Move the iterator of lower range [a..b], but not
                // the upper range as it may contain elements that will collide
                // with the next iterator. So, if mine=[0..115] and
                // theirs=[115..200], then intersection is 115 and move mine
                // but not theirs as theirs may collide with the next range
                // in thisIter.
                // move both iterators to next ranges
                if (mine.startsAfterNonDisjoint(theirs)) {
                    j++;
                } else if (theirs.startsAfterNonDisjoint(mine)) {
                    i++;
                }
            }
        }

        return intersection;
    }

    bool contains(size_t el) const { return contains(symbolToNumeric(el)); }
    bool contains(ssize_t el) const {
        if (_ranges.empty() || el < _ranges.front().start ||
            el > _ranges.back().end) {
            return false;
        }

        return std::binary_search(_ranges.begin(), _ranges.end(), Range(el, el),
                                  [](const Range &lhs, const Range &rhs) {
                                      return lhs.end < rhs.start;
                                  });
    }
    bool empty() const { return _ranges.empty(); }
    ssize_t getSingleElement() const {
        if (_ranges.size() == 1) {
            if (_ranges[0].start == _ranges[0].end) {
                return _ranges[0].start;
            }
        }

        return -1;
    }

    ssize_t getMaxElement() const {
        if (_ranges.empty()) {
            return -1;
        }

        return _ranges.back().end;
    }

    ssize_t getMinElement() const {
        if (_ranges.empty()) {
            return -1;
        }

        return _ranges.front().start;
    }

    std::vector<Range> const &getRanges() const { return _ranges; }

    // TODO: (RangeSet) hashCode
    size_t hashCode() const { return 0; }

    bool operator==(const RangeSet &other) const {
        if (_ranges.empty() && other._ranges.empty())
            return true;

        if (_ranges.size() != other._ranges.size())
            return false;

        return std::equal(_ranges.begin(), _ranges.end(),
                          other._ranges.begin());
    }

    std::string toString() const { return toString(false); }

    std::string toString(bool elemAreChar) const {
        if (_ranges.empty()) {
            return "{}";
        }

        std::stringstream ss;
        size_t effectiveSize = size();
        if (effectiveSize > 1) {
            ss << "{";
        }

        bool firstEntry = true;
        for (const auto &interval : _ranges) {
            if (!firstEntry)
                ss << ", ";
            firstEntry = false;

            ssize_t a = interval.start;
            ssize_t b = interval.end;
            if (a == b) {
                if (a == -1) {
                    ss << "<EOF>";
                } else if (elemAreChar) {
                    ss << "'" << static_cast<char>(a) << "'";
                } else {
                    ss << a;
                }
            } else {
                if (elemAreChar) {
                    ss << "'" << static_cast<char>(a) << "'..'"
                       << static_cast<char>(b) << "'";
                } else {
                    ss << a << ".." << b;
                }
            }
        }
        if (effectiveSize > 1) {
            ss << "}";
        }

        return ss.str();
    }

    // std::string elementName(const valu)

  public:
    size_t size() const {
        size_t result = 0;
        for (const auto &range : _ranges) {
            result += size_t(range.end - range.start + 1);
        }
        return result;
    }
    std::vector<ssize_t> toList() const {
        std::vector<ssize_t> result;
        for (const auto &range : _ranges) {
            ssize_t a = range.start;
            ssize_t b = range.end;
            for (ssize_t v = a; v <= b; v++) {
                result.push_back(v);
            }
        }
        return result;
    }
    std::set<ssize_t> toSet() const {
        std::set<ssize_t> result;
        for (const auto &range : _ranges) {
            ssize_t a = range.start;
            ssize_t b = range.end;
            for (ssize_t v = a; v <= b; v++) {
                result.insert(v);
            }
        }
        return result;
    }
    ssize_t get(size_t i) const {
        size_t index = 0;
        for (const auto &range : _ranges) {
            ssize_t a = range.start;
            ssize_t b = range.end;
            for (ssize_t v = a; v <= b; v++) {
                if (index == i) {
                    return v;
                }
                index++;
            }
        }
        return -1;
    }
    void remove(size_t el) { remove(symbolToNumeric(el)); }
    void remove(ssize_t el) {
        for (size_t i = 0; i < _ranges.size(); ++i) {
            Range &range = _ranges[i];
            ssize_t a = range.start;
            ssize_t b = range.end;
            if (el < a) {
                break; // list is sorted and el is before this interval; not
                       // here
            }

            // if whole interval x..x, rm
            if (el == a && el == b) {
                _ranges.erase(_ranges.begin() + (long)i);
                break;
            }
            // if on left edge x..b, adjust left
            if (el == a) {
                range.start++;
                break;
            }
            // if on right edge a..x, adjust right
            if (el == b) {
                range.end--;
                break;
            }
            // if in middle a..x..b, split interval
            if (el > a && el < b) { // found in this interval
                ssize_t oldb = range.end;
                range.end = el - 1; // [a..x-1]
                add(el + 1, oldb);  // add [x+1..b]

                break; // ml: not in the Java code but I believe we also should
                       // stop searching here, as we found x.
            }
        }
    }

  private:
    void addItems() { /* No-op */ }
};
} // namespace tagfilterdb::support

namespace std {
using tagfilterdb::support::RangeSet;

template <> struct hash<RangeSet> {
    size_t operator()(const RangeSet &x) const { return x.hashCode(); }
};
} // namespace std