#pragma once

#include "bitset.h"
#include <cstring>
#include <iostream>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "json.hpp"

using namespace nlohmann;

namespace tagfilterdb {
    class PageHeapManager;
    constexpr size_t MINIMUM_FILE_BYTES = 1; 
    constexpr int MIN_SIZE = 32;

    using PageIDType = long;
    using OffsetType = int;
    
    struct Flag {
            bool flagAssigned;
            bool flagIsAppend;
    };


   struct JsonRecord {
        Flag flag_;
        int size_;
        std::string jsonStream_;

        JsonRecord() : flag_(), size_(0), jsonStream_() {}

        void AddStream(const char* stream, int size) {
            if (stream != nullptr && size > 0) {
                jsonStream_.append(stream, size);
                size_ += size;
            }
        }

        json ParseJson() {
            return json::parse(jsonStream_);
        }
    };

    class PageHeap {

        struct FreeList {
            int offset = -1;
            int size = 0;
            FreeList* next;
            FreeList* prev;
        };


        class Iterator {
            PageHeap* page_;
            int offset_; 
            public: 
                Iterator(PageHeap *page, OffsetType offset) : page_(page), offset_(offset) {}

            void operator++() {
                offset_ = page_->nextOffset(offset_);
            }
            
            bool operator==(Iterator other) {
                return offset_ == other.offset_;
            }

            bool operator!=(Iterator other) {
                return offset_ != other.offset_;
            }

            int operator*() {
                return offset_;
            }
        };

    private:
        PageIDType PageId_;    
        char* page_;   

        FreeList* freeListH_;
        int listSize;        

        size_t maxPageBytes_;
        OffsetType lastOffset_;

        int dataCount_;

        friend PageHeapManager;
    public:
        int getMetaDataSize() {
            return   sizeof(PageIDType)           // for PageIDType
                   + sizeof(listSize)             // for List Size
                   + sizeof(OffsetType)*          // for offsets of free space (worst case is half of the data)
                    ((dataCount_ + 1)/ 2 )       
                   + sizeof(OffsetType)           // for lastOffset
                   + sizeof(dataCount_)           // for dataCount
                   ; 
        }

        PageHeap(PageIDType pageId, size_t maxPageBytes) : 
            PageId_(pageId), 
            maxPageBytes_(maxPageBytes), dataCount_(0),
            listSize(0) {
            page_ = new char[maxPageBytes];
            std::memset(page_, 0, maxPageBytes); 
            lastOffset_ = 0;

            freeListH_ = new FreeList;
            freeListH_->offset;
            freeListH_->next = freeListH_;
            freeListH_->prev = freeListH_;

            // Init First Free List with Size = MaxBytes - MetaData
            FreeList* firstList = new FreeList;
            firstList->offset = 0;
            firstList->size = maxPageBytes_ - getMetaDataSize();
            firstList->next = freeListH_;
            firstList->prev = freeListH_;

            freeListH_->next = firstList;
            freeListH_->prev = firstList;
            listSize++;
        }

        FreeList* FindFreeFit(int dataSize) {
            FreeList* curr = freeListH_->next;
            while (curr != freeListH_) {
                if (curr->size >= dataSize) {
                    return curr;  
                }
                curr = curr->next;
            }
            return nullptr;
        }

    //     bool isFull() {
    //         FreeList* curr = freeListH_->next;
    //         while (curr != freeListH_) {
    //             if (curr->size > 0) {
    //                 return false;
    //             }
    //             curr = curr->next;
    //         }
    //         return true;
    //     }

    //    OffsetType FindFreeFirst(int dataSize) {
    //         FreeList* curr = freeListH_->next;
    //         while (curr != freeListH_) {
    //             if (curr->size >= MIN_SIZE && curr->size > 0) {
    //                 return curr->offset;
    //             }
    //             curr = curr->next;
    //         }
    //         return -1;
    //     }

        Iterator begin() {
            return Iterator(this, 0);
        }

        Iterator end() {
            return Iterator(this,lastOffset_);
        }
        
        Flag LoadFlag(OffsetType offset) {
            Flag flag;

            ReadAndUpdateOffset(offset, &flag.flagAssigned, sizeof(flag.flagAssigned));
            ReadAndUpdateOffset(offset, &flag.flagIsAppend, sizeof(flag.flagIsAppend));

            return flag;
        }

        int LoadSize(OffsetType offset) {
            int size;

            ReadData(offset + 2, &size, sizeof(size));
            return size;
        }

        void LoadData(OffsetType offset, void* data, int dataSize) {
            Flag flag = LoadFlag(offset);
            if (flag.flagIsAppend) {
                ReadData(offset + 2 + sizeof(PageIDType) + sizeof(OffsetType) + sizeof(int), &data, dataSize);
            } else {
                offset = offset + 2 + sizeof(int);
                ReadData(offset, data, dataSize);
            }
        }

        std::pair<PageIDType, OffsetType> LoadAppend(OffsetType offset, Flag* f) {
            if (f->flagIsAppend) {
              std::pair<PageIDType, OffsetType> p;
              offset += 7;
              ReadAndUpdateOffset(offset, &p.first, sizeof(PageIDType)); 
              ReadAndUpdateOffset(offset, &p.first, sizeof(OffsetType)); 
            } else {
                return {-1,-1};
            }
        }

        int getOffsetSize(OffsetType offset) {
            return 2 + sizeof(int) + LoadSize(offset); 
        }

        OffsetType nextOffset(OffsetType offset) {
            Flag f = LoadFlag(offset);
            int dataSize = LoadSize(offset);
            if (f.flagIsAppend) {
                return offset + 2 + sizeof(PageIDType) + 
                        sizeof(OffsetType)  + sizeof(int) + dataSize;
            } else {
                return offset + 2 + sizeof(int) + dataSize;
            }
        }

      void Free(int offset) {
        // Mark the block as free
        Flag flag = LoadFlag(offset);
        if (!flag.flagAssigned) return;  // Already free, skip
        bool flagAssigned = false;
        SetData(offset, &flagAssigned, sizeof(flagAssigned));

        int freedBlockSize = getOffsetSize(offset);

        FreeList* current = freeListH_->next;

        while (current != freeListH_) {
            // Merge with the previous block if adjacent
            if (nextOffset(current->offset) == offset) {
                current->size += freedBlockSize;
                int tempSize = current->size - 2 - sizeof(int);
                SetData(current->offset + 2, &tempSize, sizeof(int));

                // Merge with the next block if adjacent
                if (current->next != freeListH_ && nextOffset(current->offset) == current->next->offset) {
                    current->size += current->next->size;
                    FreeList* toDelete = current->next;
                    current->next = toDelete->next;
                    toDelete->next->prev = current;
                    delete toDelete;
                }
                return;
            }

            // Merge with the next block if adjacent
            if (offset + freedBlockSize == current->offset) {
                current->offset = offset;
                current->size += freedBlockSize;

                // Merge with the previous block if adjacent
                if (current->prev != freeListH_ && nextOffset(current->prev->offset) == current->offset) {
                    current->prev->size += current->size;
                    current->prev->next = current->next;
                    current->next->prev = current->prev;
                    delete current;
                }
                return;
            }

            // Stop at the first block with a higher offset
            if (current->offset > offset) break;

            current = current->next;
        }

        // Add a new free block
        FreeList* newFreeBlock = new FreeList;
        newFreeBlock->offset = offset;
        newFreeBlock->size = freedBlockSize;
        newFreeBlock->next = current;
        newFreeBlock->prev = current->prev;

        current->prev->next = newFreeBlock;
        current->prev = newFreeBlock;
    }

        void SetData(size_t offset, const void* data, int dataSize) {
            int tempOffset = offset;
            if (offset + dataSize > maxPageBytes_ - getMetaDataSize()) {
                throw std::out_of_range("Data exceeds page bounds.");
            }
            std::memcpy(page_ + offset, data, dataSize);
        }

        void AppendData(const void* data, int dataSize) {
            int metaDataSize = getMetaDataSize();
            if (lastOffset_ + dataSize > maxPageBytes_ - metaDataSize) {
                throw std::out_of_range("Data exceeds page bounds.");
            }

            auto node = FindFreeFit(dataSize);
            std::memcpy(page_ + node->offset, data, dataSize);

            if (node->offset == lastOffset_) {
                lastOffset_ += dataSize;
                dataCount_++;
                node->offset = lastOffset_;
                node->size = maxPageBytes_ - getMetaDataSize() - lastOffset_;
            }
        }

        void ReadData(OffsetType offset, void* buffer, int dataSize) {
            if (offset + dataSize > maxPageBytes_ - getMetaDataSize()) {
                throw std::out_of_range("Data exceeds page bounds.");
            }
            std::memcpy(buffer, page_ + offset, dataSize);
        }

        void ReadAndUpdateOffset(OffsetType& offset, void* buffer, int dataSize) {
            if (offset + dataSize > maxPageBytes_ - getMetaDataSize()) {
                throw std::out_of_range("Data exceeds page bounds.");
            }
            std::memcpy(buffer, page_ + offset, dataSize);
            offset += dataSize;
        }

        char* GetData(size_t offset) {
            return page_ + offset;
        }

        /// Get the page ID
        long GetPageId() const {
            return PageId_;
        }

        /// Set the page ID
        void SetPageId(long pageId) {
            PageId_ = pageId;
        }


        void PrintFree() {
            FreeList* curr = freeListH_->next;

            while (curr != freeListH_) {
                std::cout << "Offset: "<<  curr->offset << " Size: " << curr->size << std::endl;

                curr = curr->next;
            }
        } 
    };

    
    class PageHeapManager {
    private:
        std::vector<PageHeap> pages;    ///< Vector to store all the pages.

        size_t maxPageBytes_;
        
        long nextPageId_;           ///< Page ID for the next page to be allocated.
    friend PageHeap;
    public:
        PageHeapManager(size_t maxBytes) : nextPageId_(1) {  
            if (maxBytes < MINIMUM_FILE_BYTES) {
                maxBytes = MINIMUM_FILE_BYTES;
            }
            maxPageBytes_ = maxBytes;

            pages.push_back(PageHeap(nextPageId_, maxPageBytes_));  
        }

        ~PageHeapManager() {
            for (auto& p : pages) {
                delete []p.page_;
            }
        }

        void AddRecord(const void* record, int recordSize) {
            
        }

        PageHeap* getPage(long pageID) {
            if (pageID <= pages.size()) {
                return &pages[pageID - 1];  
            }
            return nullptr;
        }

        void PrintPageInfo() const {
            for (const auto& page : pages) {

            }
        }

        size_t Size() const {
            return pages.size();
        }

        long GetNextPageId() const {
            return nextPageId_;
        }
    };
}