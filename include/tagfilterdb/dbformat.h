#ifndef TAGFILTERDB_DB_FORMAT_H
#define TAGFILTERDB_DB_FORMAT_H

#include "dataView.h"
#include "comparator.h"
#include "coding.h"
#include <cassert>

namespace tagfilterdb {

    enum ValueType { kTypeDeletion = 0x0, kTypeValue = 0x1 };

    static const ValueType kValueTypeForSeek = kTypeValue;
    typedef uint64_t SequenceNumber;
    static const SequenceNumber kMaxSequenceNumber = ((0x1ull << 56) - 1);

    inline DataView ExtractUserKey(const DataView& internal_key) {
        assert(internal_key.size() >= 8);
        return DataView(internal_key.data(),internal_key.size() - 8);
    }

    static uint64_t PackSequenceAndType(uint64_t seq, ValueType t) {
        assert(seq <= kMaxSequenceNumber);
        assert(t <= kValueTypeForSeek);
        return (seq << 8) | t;
    }

    class InternalKeyComparator : public Comparator {
        public:
            explicit InternalKeyComparator(const Comparator* c) :
            user_comparator_(c) {}

            std::string Name() const override {
                return "tagfilterdb.InternalKeyComparator";
            }

            int Compare(const DataView& akey, const DataView& bkey  ) const override {
                int result = user_comparator_->Compare(ExtractUserKey(akey), ExtractUserKey(bkey));
                if (result == 0) {
                    const uint64_t anum = DecodeFixed64(akey.data() + akey.size() - 8);
                    const uint64_t bnum = DecodeFixed64(bkey.data() + bkey.size() - 8);
                    if (anum > bnum) {
                        result = -1;
                    } else if (anum < bnum) {
                        result = +1;
                    }
                }
                return result;
            }
            void FindShortSuccessor(std::string* key) const override {
                DataView  user_key = ExtractUserKey(*key);
                std::string tmp(user_key.data(), user_key.size());
                user_comparator_->FindShortSuccessor(&tmp);
                if (tmp.size() < user_key.size() &&
                    user_comparator_->Compare(user_key, tmp) < 0) {
                  PutFixed64(&tmp,
                             PackSequenceAndType(kMaxSequenceNumber, kValueTypeForSeek));
                  assert(this->Compare(*key, tmp) < 0);
                  key->swap(tmp);
                }
            }

            void FindShortestSeparator(std::string* start, const DataView& limit) const override {
                DataView user_start = ExtractUserKey(*start);
                DataView user_limit = ExtractUserKey(limit);
                std::string tmp(user_start.data(),user_start.size());
                user_comparator_->FindShortestSeparator(&tmp, user_limit);
                if (tmp.size() < user_start.size() &&
                    user_comparator_->Compare(user_start, tmp) < 0) {
                    PutFixed64(&tmp,
                                PackSequenceAndType(kMaxSequenceNumber, kValueTypeForSeek));
                    assert(this->Compare(*start, tmp) < 0);
                    assert(this->Compare(tmp, limit) < 0);
                    start->swap(tmp);
                }
            }

        private:
            const Comparator* user_comparator_;
    };
}

#endif