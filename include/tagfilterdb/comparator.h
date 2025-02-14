#ifndef TAGFILTER_COMPARATOR_H
#define TAGFILTER_COMPARATOR_H

#include <string>

namespace tagfilterdb {
    class DataView;

    class Comparator {
        public :
        virtual ~Comparator() = default;

        virtual int Compare(const DataView& a, const DataView& b) const = 0;

        virtual std::string Name() const = 0;

        virtual void FindShortSuccessor(std::string* key) const = 0;

        virtual void FindShortestSeparator(std::string* start, const DataView& limit) const = 0;
    };

    class BytewiseComparatorImpl : public Comparator {
        public:
        BytewiseComparatorImpl() = default;
      
        std::string Name() const override { return "tagfilterdb.BytewiseComparator"; }

        int Compare(const DataView& a, const DataView& b) const override {
            return a.compare(b);
        }

        void FindShortSuccessor(std::string* key) const override { 
            size_t n = key->size();
            for (size_t i = 0; i < n; i++) {
              const uint8_t byte = (*key)[i];
              if (byte != static_cast<uint8_t>(0xff)) {
                (*key)[i] = byte + 1;
                key->resize(i + 1);
                return;
              }
            }
        }

        void FindShortestSeparator(std::string* start,
                             const DataView& limit) const override {
            // Find length of common prefix
            size_t min_length = std::min(start->size(), limit.size());
            size_t diff_index = 0;
            while ((diff_index < min_length) &&
                ((*start)[diff_index] == limit[diff_index])) {
                diff_index++;
            }

            if (diff_index >= min_length) {
            // Do not shorten if one string is a prefix of the other
            } else {
            uint8_t diff_byte = static_cast<uint8_t>((*start)[diff_index]);
            if (diff_byte < static_cast<uint8_t>(0xff) &&
                diff_byte + 1 < static_cast<uint8_t>(limit[diff_index])) {
                (*start)[diff_index]++;
                start->resize(diff_index + 1);
                assert(Compare(*start, limit) < 0);
                }
            }
        }
    };
}

#endif