#ifndef TAGFILTERDB_MEMPOOL_H
#define TAGFILTERDB_MEMPOOL_H

#include "tagfilterdb/cache.h"
#include "tagfilterdb/heapPage.h"
#include "tagfilterdb/skiplist.h"
#include "tagfilterdb/dataView.h"
#include <set>
#include "list.h"

using namespace nlohmann;

namespace tagfilterdb {
    struct BlockAddressCmp {
        int operator()(const BlockAddress& a, const BlockAddress& b) const {
            if (a.pageID != b.pageID) {
                return a.pageID - b.pageID;
            } else {
                return a.offset - b.offset;
            }
        }
    };

    struct MemPoolOpinion {
        size_t PAGE_MAX_BYTES = 1024 * 4; 
        long CACHE_CHARGE = PAGE_MAX_BYTES * 100;
        std::string FILENAME = "memPool.tin";
    };

    class MemPool {
    public:
        ShareLRUCache<HeapPage> cache_; // Cache of pages
        HeapPageMgr manager_; // Manage page access (load/flush)

        SkipList<BlockAddress, SignableData, BlockAddressCmp> signedList_; // Holds signed data
        List<SignableData> unsignedList_; // Holds unsigned data
        List<BlockAddress> freedList_; // Holds freed blocks
        List<AdjustData> adjustList_;

        Arena* arena_; // Reference to memtable Arena

    public:
        MemPool() = delete;
        MemPool(MemPoolOpinion op, Arena* arena)
            : cache_(op.CACHE_CHARGE),
            manager_(HeapPageMgr(op.FILENAME, op.PAGE_MAX_BYTES, &cache_)),
            signedList_(BlockAddressCmp(), arena),
            unsignedList_(arena),
            freedList_(arena),
            adjustList_(arena),
            arena_(arena) {}

        SignableData* Insert(DataView data) {
            data.Align(arena_);
            SignableData* signableData = 
                unsignedList_.Add(SignableData(data,BlockAddress{0,0}));
            return signableData;
        }

        SignableData* Get(BlockAddress addr) {
            SignableData* signedData = signedList_.Get(addr);
            if (signedData != nullptr) {
                return signedData;
            }

            DataView data = manager_.FetchData(addr);

            try {
                data.Align(arena_);
                return signedList_.Insert(addr,  SignableData(data, addr));

            } catch (const std::exception& e) {
                std::cerr << "Value parsing failed: " << e.what() << std::endl;
                return nullptr;
            }
        }

        BlockAddress Delete(BlockAddress addr) {
            if (signedList_.Contains(addr)) {
                // signedList_.Remove(addr); // TODO Remove
                return *freedList_.Add(addr);
            }
            return  addr;
        }
        
        bool Flush() {
            // Free First
            std::set<PageIDType> pageSet;
            auto freedIter = freedList_.begin();  
            while (freedIter != freedList_.end())
            {
                bool b = manager_.FreeBlock(freedIter->pageID, freedIter->offset,
                                            false, &adjustList_);
                pageSet.insert(freedIter->pageID);
                ++freedIter;
            }

            for (auto& pageID : pageSet) {
                if (manager_.MayCompact(pageID)) {
                    manager_.Compact(pageID, &adjustList_);                    
                }
            }

            // Sign the unsigned data 
            auto unsignedIter = unsignedList_.begin();
            while (unsignedIter != unsignedList_.end())
            {
                BlockAddress signedAddr = 
                     manager_.AddRecord(unsignedIter->data.data, unsignedIter->data.size, &adjustList_);
                unsignedIter->addr = signedAddr;
                ++unsignedIter;
            }

            manager_.Flush();
            return true;
        }

        void ClearAdjust() {
            auto iter = adjustList_.begin();
            while (iter != adjustList_.end()) {
                delete []iter->sdata.data;
                ++iter;
            }
        }

        List<AdjustData>* GetAdjust() {
            return &adjustList_;
        }
    };
}

#endif
