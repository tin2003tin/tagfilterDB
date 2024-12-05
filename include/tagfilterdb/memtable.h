#ifndef TAGFILTERDB_MEMTABLE_H
#define TAGFILTERDB_MEMTABLE_H

#include "arena.h"
#include "spatialIndex.h"

namespace tagfilterdb {
    class MemTable {
        public: 
        explicit MemTable(SpatialIndexOptions spo) : m_sp(spo, &m_arena) {}
        MemTable(const MemTable&) = delete;
        MemTable& operator=(const MemTable&) = delete;

        bool InsertSpiral(BBManager::BB &b, Interface *data) {
            if (data == nullptr) {
                return false;
            }
            Interface* dataArena = data->Align(&m_arena);
            return m_sp.Insert(b, dataArena);
        }

        Arena* GetArena() {
            return &m_arena;
        }

        SpatialIndex* GetSPI() {
            return &m_sp;
        }

    private:
        Arena m_arena;
        SpatialIndex m_sp;
    };
}

#endif