#ifndef TAGFILTERDB_MEMPOOL_H
#define TAGFILTERDB_MEMPOOL_H

#include "tagfilterdb/cache.h"
#include "tagfilterdb/pageH.h"
#include "tagfilterdb/skiplist.h"
#include "tagfilterdb/dataView.h"
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
        size_t PAGE_MAX_BYTPES = 1024 * 4; 
        long CACHE_CHARGE = PAGE_MAX_BYTPES * 100;
    };

    struct UnsignedData {
        DataView data;
        BlockAddress addr;
    };

    class MemPool {
    public:
        ShareLRUCache<PageHeap> cache_; // Cache of pages
        PageHeapManager manager_; // Manage page access (load/flush)

        SkipList<BlockAddress, DataView, BlockAddressCmp> signedList_; // Holds signed data
        List<UnsignedData> unsignedList_; // Holds unsigned data
        List<BlockAddress> freedList_; // Holds freed blocks
        List<DataView> adjustList_;

        Arena* arena_; // Reference to memtable Arena

    public:
        MemPool() = delete;
        MemPool(MemPoolOpinion op, Arena* arena)
            : cache_(op.CACHE_CHARGE),
            manager_(PageHeapManager(op.PAGE_MAX_BYTPES, &cache_)),
            signedList_(BlockAddressCmp(), arena),
            unsignedList_(arena),
            freedList_(arena),
            adjustList_(arena),
            arena_(arena) {}

        UnsignedData* Insert(DataView data) {
            data.Align(arena_);
            UnsignedData* unsignedData = 
                unsignedList_.Add(UnsignedData{data,BlockAddress{0,0}});
            return unsignedData;
        }

        DataView* Get(BlockAddress addr) {
            DataView* signedData = signedList_.Get(addr);
            if (signedData != nullptr) {
                return signedData;
            }

            DataView data = manager_.FetchData(addr);

            try {
                data.Align(arena_);
                return signedList_.Insert(addr,data);

            } catch (const std::exception& e) {
                std::cerr << "Value parsing failed: " << e.what() << std::endl;
                return nullptr;
            }
        }

        BlockAddress Delete(BlockAddress addr) {
            return *freedList_.Add(addr);    
        }
        
        bool Flush() {
            bool isCompact = false;
            // Free First
            auto freedIter = freedList_.begin();  
            while (freedIter != freedList_.end())
            {
                bool b = manager_.FreeBlock(freedIter->pageID, freedIter->offset);
                if (b) isCompact = true;
                ++freedIter;
            }

            // Sign the unsigned data 
            auto unsignedIter = unsignedList_.begin();
            while (unsignedIter != unsignedList_.end())
            {
                BlockAddress signedAddr = 
                     manager_.AddRecord(unsignedIter->data.data, unsignedIter->data.size);
                unsignedIter->addr = signedAddr;
                ++unsignedIter;
            }
            
            return isCompact;
        }
    };
}

#endif
