#ifndef TAGFILTERDB_MEMTABLE_H
#define TAGFILTERDB_MEMTABLE_H

#include "arena.h"

namespace tagfilterdb {
    template <typename Comparator>
    class MemTable {
        explicit Memtable(const Comparator& comp) {

        }

        MemTable(const MemTable&) = delete;
        MemTable& operator=(const MemTable&) = delete;

    private:
        int refs_;
        Arena arena_;
        //  table_;
    };
}

#endif