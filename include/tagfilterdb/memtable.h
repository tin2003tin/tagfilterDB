#ifndef TAGFILTERDB_MEMTABLE_H
#define TAGFILTERDB_MEMTABLE_H

#include "arena.h"
#include "spatialIndex.h"
#include "memPool.h"
#include "dataView.h"
#include "jsonMgr.h"

namespace tagfilterdb {
    class MemTable {
        public: 
        explicit MemTable(SpatialIndexOptions sop, 
                          MemPoolOpinion mop = MemPoolOpinion(), 
                          JsonMgrOp jop = JsonMgrOp())
         : memPool_(mop, &arena_), sp_(sop, &arena_,&memPool_), jsonMgr_(jop)  {}
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

        JsonMgr* GetJsonMgr() {
            return &jsonMgr_;
        }

        bool Flush() {
            memPool_.Flush();
            auto adjustData = memPool_.GetAdjust();
            auto iter = adjustData->begin();
            while (iter != adjustData->end()) {
                // get new bb 
                // Fix the R tree Offset !! 
                ++iter;
            }
            sp_.flush();
            std::cout <<  "Memory Usage: " << arena_.MemoryUsage() << std::endl;
            // TODO: WAL
        }

        void Load() {
            sp_.Load();
            memPool_.manager_.Load();
            // TODO: jsonMgr load/Save
        }

        // Save Op

    private:
        Arena arena_;
        SpatialIndex sp_;
        MemPool memPool_; 
        JsonMgr jsonMgr_;
    };
}

#endif