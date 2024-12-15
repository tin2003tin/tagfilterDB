#ifndef TAGFILTERDB_MEMTABLE_H
#define TAGFILTERDB_MEMTABLE_H

#include "arena.h"
#include "spatialIndex.h"
#include "memPool.h"
#include "dataView.h"

namespace tagfilterdb {
    class MemTable {
        public: 
        explicit MemTable(SpatialIndexOptions sop, MemPoolOpinion mop)
         : memPool_(mop, &arena_), sp_(sop, &arena_,&memPool_)  {}
        MemTable(const MemTable&) = delete;
        MemTable& operator=(const MemTable&) = delete;

        Arena* GetArena() {
            return &arena_;
        }

        SpatialIndex* GetSPI() {
            return &sp_;
        }

        MemPool* GetMempool() {
            return &memPool_;
        }

    private:
        Arena arena_;
        SpatialIndex sp_;
        MemPool memPool_; 
    };
}

#endif