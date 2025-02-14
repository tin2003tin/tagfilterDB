#include <iostream>
#include "tagfilterdb/dbformat.h"

using namespace tagfilterdb;

int main() {
    BytewiseComparatorImpl user_cmp;
    InternalKeyComparator internal_cmp(&user_cmp);

    std::string user_key1 = "aaa";
    std::string user_key2 = "rab";

    uint64_t seq1 = 100;
    uint64_t seq2 = 100;

    std::string internal_key_1 = user_key1 + EncodeFixed64ToString(seq1);
    std::string internal_key_2 = user_key2 + EncodeFixed64ToString(seq2);

    int cmp_result = internal_cmp.Compare(internal_key_1, internal_key_2);

    std::cout << "Comparison Result: " << cmp_result << std::endl;

    std::cout << "Before: " << internal_key_1 << std::endl;

    internal_cmp.FindShortSuccessor(&internal_key_1);

    std::cout << "After: " << internal_key_1 << std::endl;

    return 0;
}
