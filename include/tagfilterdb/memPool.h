#ifndef TAGFILTERDB_MEMPOOL_H
#define TAGFILTERDB_MEMPOOL_H

#include "tagfilterdb/cache.h"
#include "tagfilterdb/pageH.h"
#include "tagfilterdb/skiplist.h"
#include "json.hpp"
#include "list.h"

using namespace nlohmann;

namespace tagfilterdb {

    struct BlockAddressComparator {
        int operator()(const BlockAddress& a, const BlockAddress& b) const {
            if (a.first != b.first) {
                return a.first - b.first;
            } else {
                return a.second - b.second;
            }
        }
    };
    
    enum ObjectState {
        Signed,
        UnSigned,
        Freed,
    };

    struct JsonObject {
        ObjectState state_;
        json data_;

        JsonObject(ObjectState state, json data) : state_(state), data_(std::move(data)) {}

        bool Assign(Arena* arena) {
            // [TODO] Write in arena with json Size and State size
            return true;
        }

        static JsonObject* newObj(ObjectState state, json data, Arena* arena) {
            // [TODO] Create new JsonObject and assign
            return new JsonObject(state, std::move(data));
        }
    };

    struct MemPoolOpinion {
        size_t PAGE_MAX_BYTPES = 1024 * 4; 
        long CACHE_CHARGE = PAGE_MAX_BYTPES * 100;
    };

    class MemPool {
        
    public:
        ShareLRUCache<PageHeap* > cache_;  // Cache of pages
        PageHeapManager manager_;         // Manage page access (load/flush)
        
        SkipList<BlockAddress, JsonObject*, BlockAddressComparator> signedList_;  // Holds signed data
        SkipList<BlockAddress, JsonObject*, BlockAddressComparator> unsignedList_;  // Holds unsigned data
        List<BlockAddress> freedList_;  // Holds freed blocks (simple map for freed blocks)
        Arena* arena_; // Reference to memtable Arena
        BlockAddressComparator cmp_;
    
    public:
        MemPool() = delete;
        MemPool(MemPoolOpinion op, Arena* arena)
            : cache_(op.CACHE_CHARGE),
              manager_(PageHeapManager(op.PAGE_MAX_BYTPES, &cache_)),
              cmp_(BlockAddressComparator()),
              signedList_(cmp_, arena), 
              unsignedList_(cmp_, arena),
              arena_(arena) {}

        bool Insert(BlockAddress addr, ObjectState state, json data) {
            JsonObject* obj = JsonObject::newObj(state, data, arena_);
            if (state == Signed) {
                signedList_.Insert(addr, obj);
            } else if (state == UnSigned) {
                unsignedList_.Insert(addr, obj);
            } else {
                freedList_.Add(addr);
            }
            return true;
        }

        bool MarkFree(BlockAddress addr) {
            JsonObject* obj = GetBlock(addr, Signed);
            if (obj) {
                // Move signed block to freed state
                obj->state_ = Freed;
                freedList_.Add(addr);  // Add freed block to the freed list
                signedList_.Remove(addr);  // Remove from signed list
                return true;
            }
            return false;
        }

        JsonObject* LoadData(BlockAddress addr) {
            // Find in Signed SkipList
            if (signedList_.Contains(addr)) {
                return signedList_.Get(addr);
            }
            // if not found find the page in cache and store in Signed SkipList
            auto n = cache_.Get(std::to_string(addr.first));
            if (n != nullptr) {
                
            }
           
            // if not found pagemanger load that offset
              
        } 

        JsonObject* GetBlock(BlockAddress addr, ObjectState state) {
            if (state == Signed) {
                return signedList_.Get(addr);
            } else if (state == UnSigned) {
                return unsignedList_.Get(addr);
            } else {
                return nullptr;
            }
        }

        bool UpdateBlockState(BlockAddress addr, ObjectState newState, json newData) {
            JsonObject* obj = GetBlock(addr, Signed);
            if (!obj) {
                return false;  // Block not found
            }
            
            // Handle the transition of states
            if (newState == Signed) {
                // Move unsigned block to signed or just update if it's already signed
                if (obj->state_ == UnSigned) {
                    unsignedList_.Remove(addr);
                    obj = JsonObject::newObj(Signed, newData, arena_);
                    signedList_.Insert(addr, obj);
                } else if (obj->state_ == Signed) {
                    obj->data_ = newData;  // Update signed block data
                }
            } else if (newState == UnSigned) {
                // Move signed block to unsigned or just update if it's already unsigned
                if (obj->state_ == Signed) {
                    signedList_.Remove(addr);
                    obj = JsonObject::newObj(UnSigned, newData, arena_);
                    unsignedList_.Insert(addr, obj);
                } else if (obj->state_ == UnSigned) {
                    obj->data_ = newData;  // Update unsigned block data
                }
            } else if (newState == Freed) {
                // Moving signed block to freed state
                if (obj->state_ == Signed) {
                    signedList_.Remove(addr);
                    obj->state_ = Freed;
                    freedList_.Add(addr);
                }
                // Freed blocks cannot be moved
            }
            return true;
        }

        bool FreeBlock(BlockAddress blockAddress) {
            JsonObject* obj = GetBlock(blockAddress, Freed);
            if (obj) {
                freedList_.Add(blockAddress);  // Add to freed list
                return true;
            }
            return false;
        }

        // You can add other methods for cleanup, querying, etc.
    };
}

#endif
