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
                // fix just exist in signed skip list
                auto sData = memPool_.Get(iter->oldAddr, false);
                if (sData) {
                    sData->addr = iter->newAddr;
                } else {
                    // Fix the R tree Offset !! 
                    BBManager::BB box;
                    std::vector<BBManager::BB::Edge> ve;
                    json jData = JsonMgr::ToJson(iter->sdata);
                    std::string s = jData.dump();
                    jsonMgr_.GetPairDouble(sp_.getOption()->DNAME, ve, jData);
                    box = BBManager::CreateBox(ve,sp_.getOption()->DIMENSION);

                    bool adjusted = sp_.AdjustOffset(box,iter->oldAddr,iter->newAddr);
                    assert(adjustData);
                    box.Destroy();
                }
               
                ++iter;
            }
            sp_.flush();
            // TODO: WAL
            // TODO: Cache
            return true;
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