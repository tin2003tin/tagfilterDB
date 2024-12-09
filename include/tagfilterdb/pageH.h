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
    constexpr int MIN_SIZE = 2 + 4;

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
            OffsetType offset_ = -1;
            int blockSize_ = 0;
            FreeList* next_;
            FreeList* prev_;
            FreeList(OffsetType offset, int blackSize, 
                    FreeList* next, FreeList* prev) : offset_(offset), 
                    blockSize_(blackSize), next_(next), prev_(prev) {}
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
        int getMetaDataSize(int dataCount) {
            return sizeof(PageIDType)                                   // for PageIDType
                + sizeof(listSize)                                      // for List Size
                + (sizeof(OffsetType) + sizeof(int)) *((dataCount)/ 2)  // for offsets of free space (worst case is half of the data)
                + sizeof(OffsetType)                                    // for lastOffset
                + sizeof(dataCount_)                                    // for dataCount
                ; 
        }

        PageHeap(PageIDType pageId, size_t maxPageBytes) : 
            PageId_(pageId), 
            maxPageBytes_(maxPageBytes), dataCount_(0),
            listSize(0) {
            page_ = new char[maxPageBytes];
            std::memset(page_, 0, maxPageBytes); 
            lastOffset_ = 0;

            freeListH_ = new FreeList(0,0, nullptr, nullptr);
            freeListH_->next_ = freeListH_;
            freeListH_->prev_ = freeListH_;

            // Init First Free List with Size = MaxBytes - MetaData
            FreeList* firstList = new FreeList(0,maxPageBytes_ - getMetaDataSize(dataCount_),
                                                freeListH_,freeListH_);

            freeListH_->next_ = firstList;
            freeListH_->prev_ = firstList;
            listSize++;
        }

        FreeList* FindFree(int dataSize) {
            FreeList* curr = freeListH_->next_;
            while (curr != freeListH_) {
                if (curr->blockSize_ >= dataSize) {
                    return curr;  
                }
                curr = curr->next_;
            }
            return freeListH_->prev_;
        }

        bool isFull() {
            return lastOffset_ == maxPageBytes_ - getMetaDataSize(dataCount_);
        }

        OffsetType End() {
            return maxPageBytes_ - getMetaDataSize(dataCount_);
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
            offset = offset + 2 + sizeof(int);
            ReadData(offset, data, dataSize);
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

      void Free(OffsetType offset) {
        if (offset >= lastOffset_) {
            return;
        }
        // Mark the block as free
        Flag flag = LoadFlag(offset);
        if (!flag.flagAssigned) return;  // Already free, skip
  
        bool flagAssigned = false;
        SetData(offset, &flagAssigned, sizeof(flagAssigned));

        int freedBlockSize = getOffsetSize(offset);

        FreeList* current = freeListH_->next_;

        while (current != freeListH_) {
            if (current->offset_ > offset) {
                break;
            }
            current = current->next_;   
        }

        bool isMergeLeft = false;
        if (current->prev_ != freeListH_ &&
            current->prev_->offset_ + current->prev_->blockSize_ == offset) {
            current->prev_->blockSize_ += freedBlockSize;
            isMergeLeft = true;
        }

        if (offset + freedBlockSize == current->offset_) {
            if (isMergeLeft) {
                if (current->offset_ == lastOffset_) {
                    lastOffset_ = current->prev_->offset_;
                }
                current->prev_->blockSize_ += current->blockSize_;
                current->prev_->next_ = current->next_;
                current->next_->prev_ = current->prev_;
                
                int acutalSize = current->prev_->blockSize_ - 6; 
                SetData(current->prev_->offset_ + 2, &acutalSize, sizeof(int));

                delete current;
                listSize--;
                dataCount_ -= 2;
            } else {
                if (current->offset_ == lastOffset_) {
                    lastOffset_ = offset;
                }

                current->offset_ = offset;
                current->blockSize_ += freedBlockSize;
               
                dataCount_ -= 1;
                int acutalSize = current->blockSize_ - 6; 
                SetData(current->offset_ + 2, &acutalSize, sizeof(int));
            }
                
            return;
        }

        if (isMergeLeft) {
            int acutalSize = current->prev_->blockSize_ - 6; 
            dataCount_ -= 1;
            SetData(current->prev_->offset_ + 2, &acutalSize, sizeof(int));
            return;
        }

        // Add a new free block
        FreeList* newFreeBlock = nullptr;
        newFreeBlock = new FreeList(offset,freedBlockSize,current,current->prev_);

        current->prev_->next_ = newFreeBlock;
        current->prev_ = newFreeBlock;
        listSize++;
    }

    void SetData(size_t offset, const void* data, int dataSize) {
            int tempOffset = offset;
            if (offset + dataSize > maxPageBytes_ - getMetaDataSize(dataCount_)) {
                throw std::out_of_range("Data exceeds page bounds.");
            }
            std::memcpy(page_ + offset, data, dataSize);
    }

    void AddDataBlockAt(FreeList* node, const void* datablock, int datablockSize) {
        assert(datablockSize >= MIN_SIZE);
        assert(node->blockSize_ >= datablockSize);
        assert(node);

        std::memcpy(page_ + node->offset_, datablock, datablockSize);

        if (node->offset_ == lastOffset_) {
                lastOffset_ += datablockSize;
                dataCount_++;
                node->offset_ = lastOffset_;
                node->blockSize_ = maxPageBytes_ - getMetaDataSize(dataCount_) - lastOffset_;
                return;
            }
        if (node->blockSize_ - datablockSize > 0) {
                node->offset_ += datablockSize;
                node->blockSize_ = node->blockSize_ - datablockSize;
                
                int acutalSize = node->blockSize_ - 6;
                bool isAssigned = false;
                SetData(node->offset_ + 2, &acutalSize, sizeof(int));
            } else {
                node->prev_->next_ = node->next_;
                node->next_->prev_ = node->prev_;

                delete node;   

                listSize--;
        }
    }

    void ReadData(OffsetType offset, void* buffer, int dataSize) {
            int temp = maxPageBytes_ - getMetaDataSize(dataCount_);
            if (offset + dataSize > maxPageBytes_ - getMetaDataSize(dataCount_)) {
                throw std::out_of_range("Data exceeds page bounds.");
            }
            std::memcpy(buffer, page_ + offset, dataSize);
    }

    void ReadAndUpdateOffset(OffsetType& offset, void* buffer, int dataSize) {
            if (offset + dataSize > maxPageBytes_ - getMetaDataSize(dataCount_)) {
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
            FreeList* curr = freeListH_->next_;

            while (curr != freeListH_) {
                std::cout << "Page: " << PageId_ << " Offset: "<<  curr->offset_ << " Size: " << curr->blockSize_ << std::endl;

                curr = curr->next_;
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
        
        class Iterator {
            PageHeapManager* pm_;
            int pageID_;
            int offset_;

        public:
            // Constructors
            Iterator(PageHeapManager* pm)
                : pm_(pm), pageID_(1), offset_(0) {
                skipUnassigned();
            }

            Iterator(PageHeapManager* pm, int pageID, int offset)
                : pm_(pm), pageID_(pageID), offset_(offset) {
            }

            // Advance to the next valid record
            void operator++() {
                PageHeap* page = pm_->getPage(pageID_);
                bool isAppend = false;  
                while (true) {
                    // Load the flag for the current offset
                    Flag flag = page->LoadFlag(offset_);
                    
                    // If `flagIsAppend` is set, continue to the next page
                    if (flag.flagIsAppend) {
                        isAppend = true;
                        pageID_++;
                        if (pageID_ > pm_->pages.size()) {
                            offset_ = -1; // Mark as end of data
                            return;
                        }
                        page = pm_->getPage(pageID_);

                        offset_ = 0;
                    } else {
                        break;
                    } 
                }
                
                offset_ = page->nextOffset(offset_);
                skipUnassigned();
            }

            // Compare iterators
            bool operator==(const Iterator& other) const {
                return pageID_ == other.pageID_ && offset_ == other.offset_;
            }

            bool operator!=(const Iterator& other) const {
               return !(*this == other);
            }

            // Dereference operator to get current position
            std::pair<int, int> operator*() const {
                return {pageID_, offset_};
            }

        private:
            // Skip over free and unassigned blocks
            void skipUnassigned() {
                PageHeap* page = pm_->getPage(pageID_);
                if (offset_ == page->lastOffset_) {
                    if (pageID_ + 1 <= pm_->Size()) {
                        pageID_++;
                        offset_ = 0;
                        skipUnassigned();
                    }
                    return;
                }
                auto curr = page->freeListH_->next_;
                while (curr != page->freeListH_ && offset_ <= curr->offset_ ) {
                    if (offset_ == curr->offset_) {
                        offset_ += curr->blockSize_;
                        if (offset_ == page->lastOffset_) {
                           if (pageID_ + 1 <= pm_->Size()) {
                                pageID_++;
                                offset_ = 0;
                                skipUnassigned();
                            }
                        }
                        return;
                    }
                    curr = curr->next_;
                }
            }
        };

        friend Iterator;

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

    char* Block(Flag flag, const void* record, int recordSize) {
        int dataSize = sizeof(flag.flagAssigned) + sizeof(flag.flagIsAppend) +
                   sizeof(recordSize) + recordSize;
        char* buffer = new char[dataSize]; 
        int offset = 0;
        std::memcpy(buffer + offset, &flag.flagAssigned, sizeof(flag.flagAssigned));
        offset += sizeof(flag.flagAssigned);
        std::memcpy(buffer + offset, &flag.flagIsAppend, sizeof(flag.flagIsAppend));
        offset += sizeof(flag.flagIsAppend);
        std::memcpy(buffer + offset, &recordSize, sizeof(recordSize));
        offset += sizeof(recordSize);
        std::memcpy(buffer + offset, record, recordSize);

        return buffer;
    }

    int BlockSize(int recordSize) {
        return 2 + sizeof(int) + recordSize;
    }

    int BlockToDataSize(int dataBlock) {
        return dataBlock - 2 - sizeof(int); 
    }

    void AddRecord(const char* record, int recordSize) {
        Flag flag{true,false};
        int blockSize = BlockSize(recordSize);
        int index = 0;
        while (true) {
            auto node = pages[index].FindFree(blockSize); 
            bool isAppendable = false;
            if (node == pages[index].freeListH_->prev_) {
                isAppendable = true;
            }
            if (RecursivelyAddRecord(node->offset_,index,record,0,recordSize, isAppendable)) {
                break;
            }
            index = getNextPageIndex(index);
        }
    }

    bool RecursivelyAddRecord(int startOffset, long index, const void* record, 
                              OffsetType offset, int recordSize, bool isAppendable) {
        auto node = pages[index].freeListH_->next_;
        if (node->offset_ != startOffset) {
            return false;
        }
        if (node->blockSize_ < MIN_SIZE) {
            return false;
        }
        Flag flag{true,false};
        int blockSize = BlockSize(recordSize);
        int diff = pages[index].getMetaDataSize(pages[index].dataCount_ + 1) - 
                       pages[index].getMetaDataSize(pages[index].dataCount_);
        if (!isAppendable) {
            diff = 0;
        }
        if(node->blockSize_ - diff >= blockSize) {
            const char* partialRecord = static_cast<const char*>(record) + offset;
            char* dataBlock = Block(flag,partialRecord, recordSize);
            pages[index].AddDataBlockAt(node, dataBlock, blockSize);
            delete []dataBlock;
            return true;
        }

        if (node->blockSize_ - diff < MIN_SIZE) {
            return false;   
        }

        if (!isAppendable) {
            return false;
        }

        int nextIndex = getNextPageIndex(index);
        int splitSize = recordSize - BlockToDataSize(node->blockSize_) + diff;
        if (RecursivelyAddRecord(0, nextIndex, record, 0 + BlockToDataSize(node->blockSize_ - diff), splitSize, true)) {
                    flag.flagIsAppend = true;
                    char* dataBlock = Block(flag, record, BlockToDataSize(node->blockSize_ - diff));
                    pages[index].AddDataBlockAt(node, dataBlock, node->blockSize_ - diff);
                    delete []dataBlock;
                    return true;
        } else {
            return false;
        }
    }

    std::pair<char*, int> GetData(long pageID, int offset) {
        assert(pageID <= pages.size());
        PageHeap* page = getPage(pageID);
        assert(offset < page->lastOffset_);

        int totalSize = 0;
        std::vector<std::pair<PageHeap*, int>> dataBlocks;

        // Collect all data blocks
        while (pageID <= pages.size()) {
            PageHeap* page = getPage(pageID);
            Flag flag = page->LoadFlag(offset);
            assert(flag.flagAssigned);

            int dataSize = page->LoadSize(offset);
            dataBlocks.emplace_back(page, offset);

            totalSize += dataSize;

            if (!flag.flagIsAppend) {
                break;
            }

            pageID += 1; // Move to the next page
            offset = 0;  // Start from the beginning of the next page
        }

        // Allocate buffer for the entire data
        char* buffer = new char[totalSize];
        int writeOffset = 0;

        // Load data into the buffer
        for (const auto& block : dataBlocks) {
            PageHeap* page = block.first;
            int dataOffset = block.second;
            int dataSize = page->LoadSize(dataOffset);

            page->LoadData(dataOffset, buffer + writeOffset, dataSize);
            writeOffset += dataSize;
        }

        return {buffer, totalSize};
    }

    void FreeData(long pageID, int offset) {
        PageHeap* page = getPage(pageID);
        Flag flag = page->LoadFlag(offset);
        assert(flag.flagAssigned);

        if (flag.flagIsAppend) {
            FreeData(pageID + 1, 0);
        }
        page->Free(offset);
    }

    long getNextPageIndex(long index) {
        if (index + 1 >= pages.size()) {
            nextPageId_++;
            pages.push_back(PageHeap(nextPageId_, maxPageBytes_));  
        }
        return index + 1;
    }

    PageHeap* getPage(long pageID) {
        if (pageID <= pages.size()) {
            return &pages[pageID - 1];  
        }
        return nullptr;
    }

    Iterator begin() {
        return Iterator(this);
    }

    Iterator end() {
        int lastOffset = pages[pages.size() - 1].lastOffset_;
        Iterator iter(this,pages.size(),lastOffset);
        return iter;
    }

    int TotalCount() {
        int count = 0;
        for (auto e : pages) {
            count += e.dataCount_;
        }
        return count;
    }

    void PrintPageInfo() {
        std::cout << "===============\n";
            for (int i = 0; i < pages.size(); i++) {
                std::cout << "Page: " << i + 1 << std::endl;
                pages[i].PrintFree();
                std::cout << pages[i].dataCount_ << std::endl;
            }
             std::cout << "===============\n";
        }

        size_t Size() const {
            return pages.size();
        }

        long GetNextPageId() const {
            return nextPageId_;
        }
    };
}