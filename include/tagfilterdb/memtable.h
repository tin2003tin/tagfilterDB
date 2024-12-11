#ifndef TAGFILTERDB_MEMTABLE_H
#define TAGFILTERDB_MEMTABLE_H

#include "arena.h"
#include "spatialIndex.h"

namespace tagfilterdb {
    template <typename Value>
    class MemTable {
        public: 
        explicit MemTable(SpatialIndexOptions spo) : m_sp(spo, &m_arena) {}
        MemTable(const MemTable&) = delete;
        MemTable& operator=(const MemTable&) = delete;

        Arena* GetArena() {
            return &m_arena;
        }

        SpatialIndex<Value>* GetSPI() {
            return &m_sp;
        }

    private:
        Arena m_arena;
        SpatialIndex<Value> m_sp;
    };
}

#endif