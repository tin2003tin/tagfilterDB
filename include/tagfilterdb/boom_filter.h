#ifndef TAGFILTERDB_BOOM_FILTER_H
#define TAGFILTERDB_BOOM_FILTER_H

#include "filter_policy.h"
#include "murmurHash.h"

namespace tagfilterdb {
    static uint32_t BloomHash(const DataView& key) {
        return support::MurmurHash::Hash(key.data(), key.size(), 0xbc9f1d34);
    }
    
    class BloomFilterPolicy : public FilterPolicy {
        public:
        explicit BloomFilterPolicy(int bits_per_key) : bits_per_key_(bits_per_key) {
            k_ = static_cast<size_t>(bits_per_key * 0.69);  // 0.69 =~ ln(2)
            if (k_ < 1) k_ = 1;
            if (k_ > 30) k_ = 30;
        }

       std::string Name() const override { return "tagfilterdb.BuiltinBloomFilter2"; }

       void CreateFilter(const DataView* keys, int n, std::string* dst) const override {
        // Compute bloom filter size (in both bits and bytes)
        size_t bits = n * bits_per_key_;
    
        // For small n, we can see a very high false positive rate.  Fix it
        // by enforcing a minimum bloom filter length.
        if (bits < 64) bits = 64;
    
        size_t bytes = (bits + 7) / 8;
        bits = bytes * 8;
    
        const size_t init_size = dst->size();
        dst->resize(init_size + bytes, 0);
        dst->push_back(static_cast<char>(k_));  // Remember # of probes in filter
        char* array = &(*dst)[init_size];
        for (int i = 0; i < n; i++) {
          // Use double-hashing to generate a sequence of hash values.
          // See analysis in [Kirsch,Mitzenmacher 2006].
          uint32_t h = BloomHash(keys[i]);
          const uint32_t delta = (h >> 17) | (h << 15);  // Rotate right 17 bits
          for (size_t j = 0; j < k_; j++) {
            const uint32_t bitpos = h % bits;
            array[bitpos / 8] |= (1 << (bitpos % 8));
            h += delta;
          }
        }
      }

       bool KeyMayMatch(const DataView& key,const DataView& filter) const override {
            const size_t len = filter.size();
            if (len < 2) return false;

            const char* array = filter.data();
            const size_t bits = (len - 1) * 8;
            const size_t k = array[len - 1];
            if (k > 30) {
                return true;
            }
            uint32_t h = BloomHash(key);
            const uint32_t delta = (h >> 17) | (h << 15);  // Rotate right 17 bits
            for (size_t j = 0; j < k; j++) {
              const uint32_t bitpos = h % bits;
              if ((array[bitpos / 8] & (1 << (bitpos % 8))) == 0) return false;
              h += delta;
            }
            return true;
       }    

        private:
        size_t bits_per_key_;
        size_t k_;
    };
}

#endif