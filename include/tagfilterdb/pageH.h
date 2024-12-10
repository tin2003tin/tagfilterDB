#pragma once

#include "bitset.h"
#include <cstring>
#include <iostream>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <functional>

#include "json.hpp"

using namespace nlohmann;

namespace tagfilterdb {
    class PageHeapManager;
    constexpr size_t MINIMUM_FILE_BYTES = 1; 
    constexpr int MIN_SIZE = 2 + 4;
    constexpr int FREE_LIST_SIZE = 5;

    using PageIDType = long;
    using OffsetType = int;
    using BlockAddress = std::pair<PageIDType,OffsetType>;
    
    #pragma pack(1)
    struct Flag {
        bool flagAssigned;
        bool flagIsAppend;
    };
    #pragma pack()

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
        int listSize_;        

        size_t maxPageBytes_;
        OffsetType lastOffset_;

        int blockSpace_;
        int blockCount_;

        friend PageHeapManager;
    public:

      class Iterator {
            PageHeap* page_;
            OffsetType offset_;

        public:
            // Constructors
            Iterator(PageHeap* page)
                : page_(page), offset_(0) {
                skipFree();
            }

            Iterator(PageHeap* page, OffsetType offset)
                : page_(page), offset_(offset) {
            }

            // Advance to the next valid record
            void operator++() {
                assert(page_);
                
                offset_ = page_->nextOffset(offset_);
                skipFree();
            }

            // Compare iterators
            bool operator==(const Iterator& other) const {
                return offset_ == other.offset_;
            }

            bool operator!=(const Iterator& other) const {
               return !(*this == other);
            }

            OffsetType operator*() const {
                return offset_;
            }

        private:
            void skipFree() {
                assert(page_);
                if (offset_ == page_->lastOffset_) {
                    return;
                }
                auto curr = page_->freeListH_->next_;
                while (curr != page_->freeListH_ && offset_ >= curr->offset_ ) {
                    if (offset_ == curr->offset_) {
                        offset_ += curr->blockSize_;
                        return;
                    }
                    curr = curr->next_;
                }
            }
        };

        int getMetaDataSize() {
            return sizeof(PageIDType)                                    // for PageIDType
                + sizeof(listSize_)                                       // for List Size
                + (sizeof(OffsetType) + sizeof(int)) *((FREE_LIST_SIZE)) // for offsets of free space (worst case is half of the data)
                + sizeof(OffsetType)                                     // for lastOffset
                + sizeof(blockSpace_)                                    // for dataCount
                + sizeof(blockCount_)  
                ; 
        }

        PageHeap(PageIDType pageId, size_t maxPageBytes) : 
            PageId_(pageId), 
            maxPageBytes_(maxPageBytes), blockSpace_(0),
            listSize_(0), blockCount_(0) {
            page_ = new char[maxPageBytes];
            std::memset(page_, 0, maxPageBytes); 
            lastOffset_ = 0;

            freeListH_ = new FreeList(0,0, nullptr, nullptr);
            freeListH_->next_ = freeListH_;
            freeListH_->prev_ = freeListH_;

            // Init First Free List with Size = MaxBytes - MetaData
            FreeList* firstList = new FreeList(0, maxPageBytes_ - getMetaDataSize(),
                                                freeListH_,freeListH_);

            freeListH_->next_ = firstList;
            freeListH_->prev_ = firstList;
            listSize_++;
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
            return lastOffset_ - maxPageBytes_ - getMetaDataSize() < MIN_SIZE;
        }

        OffsetType EndBlocks() {
            return maxPageBytes_ - getMetaDataSize();
        }
        
        Flag LoadFlag(OffsetType offset) {
            Flag flag;
            char buffer;

            ReadAndUpdateOffset(offset, &buffer, 1);
            flag.flagAssigned = static_cast<bool>(buffer);

            ReadAndUpdateOffset(offset, &buffer, 1);
            flag.flagIsAppend = static_cast<bool>(buffer);

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
            return offset + 2 + sizeof(int) + dataSize;
        }

      void Free(OffsetType offset, int blockSize) {
        if (offset >= lastOffset_) {
            return;
        }
        blockCount_--;

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
            current->prev_->blockSize_ += blockSize;
            isMergeLeft = true;
        }

        if (offset + blockSize == current->offset_) {
            if (isMergeLeft) {
                if (current->offset_ == lastOffset_) {
                    lastOffset_ = current->prev_->offset_;
                }
                current->prev_->blockSize_ += current->blockSize_;
                current->prev_->next_ = current->next_;
                current->next_->prev_ = current->prev_;

                delete current;
                listSize_--;
                blockSpace_ -= 2;
            } else {
                if (current->offset_ == lastOffset_) {
                    lastOffset_ = offset;
                }

                current->offset_ = offset;
                current->blockSize_ += blockSize;
                blockSpace_ -= 1;
            }
                
            return;
        }

        if (isMergeLeft) {
            int acutalSize = current->prev_->blockSize_ - 6; 
            blockSpace_ -= 1;
            return;
        }

        // Add a new free block
        FreeList* newFreeBlock = nullptr;
        newFreeBlock = new FreeList(offset,blockSize,current,current->prev_);

        current->prev_->next_ = newFreeBlock;
        current->prev_ = newFreeBlock;
        listSize_++;
        return;
    }

    void SetData(size_t offset, const void* data, int dataSize) {
            int tempOffset = offset;
            if (offset + dataSize > EndBlocks()) {
                throw std::out_of_range("Data exceeds page bounds.");
            }
            std::memcpy(page_ + offset, data, dataSize);
    }

    void AddDataBlockAt(FreeList* node, const void* datablock, int datablockSize) {
        assert(datablockSize >= MIN_SIZE);
        assert(node->blockSize_ >= datablockSize);
        assert(node);
        blockCount_++;
        std::memcpy(page_ + node->offset_, datablock, datablockSize);

        if (node->offset_ == lastOffset_) {
                lastOffset_ += datablockSize;
                blockSpace_++;
                node->offset_ = lastOffset_;
                node->blockSize_ = EndBlocks() - lastOffset_;
                return;
            }
        if (node->blockSize_ - datablockSize > 0) {
                node->offset_ += datablockSize;
                node->blockSize_ = node->blockSize_ - datablockSize;
                
                blockSpace_++;
                
            } else {
                node->prev_->next_ = node->next_;
                node->next_->prev_ = node->prev_;
                // TODO: 
                delete node;   

                listSize_--;
        }
    }

    void ReadData(OffsetType offset, void* buffer, int dataSize) {
            if (offset + dataSize > EndBlocks()) {
                throw std::out_of_range("Data exceeds page bounds.");
            }
            std::memcpy(buffer, page_ + offset, dataSize);
    }

    void ReadAndUpdateOffset(OffsetType& offset, void* buffer, int dataSize) {
            if (offset + dataSize > EndBlocks()) {
                throw std::out_of_range("Data exceeds page bounds.");
            }
            std::memcpy(buffer, page_ + offset, dataSize);
            offset += dataSize;
        }

        Iterator Begin() {
            return Iterator(this);
        }

        Iterator End() {
             return Iterator(this, lastOffset_);
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
            std::cout << "ListSize: " << listSize_ << std::endl;
            while (curr != freeListH_) {
                std::cout << "- Page: " << PageId_ << " Offset: "<<  curr->offset_ << " Size: " << curr->blockSize_ << std::endl;

                curr = curr->next_;
            }
        } 
    };

    class PageHeapManager {
        
    private:
        std::vector<PageHeap> pages;    ///< Vector to store all the pages.

        size_t maxPageBytes_;
        
        long lastPageId_;           ///< Page ID for the next page to be allocated.
    

    public:
        friend PageHeap;

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
                assert(page);
                bool isAppend = false;  
                while (true) {
                    // Load the flag for the current offset
                    Flag flag = page->LoadFlag(offset_);
                    
                    // If `flagIsAppend` is set, continue to the next page
                    if (flag.flagIsAppend) {
                        isAppend = true;
                        pageID_++;
                        if (pageID_ > pm_->pages.size()) {
                            assert(false);
                        }
                        page = pm_->getPage(pageID_);
                        assert(page);
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

            BlockAddress operator*() const {
                return {pageID_, offset_};
            }

        private:
            // Skip over free and unassigned blocks
            void skipUnassigned() {
                PageHeap* page = pm_->getPage(pageID_);
                assert(page);
                if (offset_ == page->lastOffset_) {
                    if (pageID_ + 1 <= pm_->Size()) {
                        pageID_++;
                        offset_ = 0;
                        skipUnassigned();
                    }
                    return;
                }
                auto curr = page->freeListH_->next_;
                while (curr != page->freeListH_ && offset_ >= curr->offset_ ) {
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
        PageHeapManager(size_t maxBytes) : lastPageId_(1) {  
            if (maxBytes < MINIMUM_FILE_BYTES) {
                maxBytes = MINIMUM_FILE_BYTES;
            }
            maxPageBytes_ = maxBytes;

            pages.push_back(PageHeap(lastPageId_, maxPageBytes_));  
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

    BlockAddress AddRecord(const char* record, int recordSize) {
        Flag flag{true,false};
        int blockSize = BlockSize(recordSize);
        int pageID = 1;
        int assignedOffset = -1;
        while (true) {
            PageHeap::FreeList* node = getPage(pageID)->FindFree(blockSize); 
            assignedOffset = node->offset_;
            bool isAppendable = false;
            if (node == getPage(pageID)->freeListH_->prev_) {
                isAppendable = true;
            }
            if (RecursivelyAddRecord(node,pageID,record,0,recordSize, isAppendable, true)) {
                break;
            }
            // Create New page ?
            if (IsCreateNewPage(pageID)) {
                // TODO Compact
                 Compact(pageID);
                CreateNewPage();
            }   
            pageID++;
        }
        assert(assignedOffset >= 0);
        return {pageID, assignedOffset};
    }

    bool RecursivelyAddRecord(PageHeap::FreeList* node, long pageID, const void* record, 
                              OffsetType offset, int recordSize, bool isAppendable, bool isFirst) {
        if (!isFirst)  {
            if (node->offset_ != 0) {
                return false;   
            }
        } 
       
        if (node->blockSize_ < MIN_SIZE) {
            return false;
        }
        Flag flag{true,false};
        int blockSize = BlockSize(recordSize);
        
        if(node->blockSize_ >= blockSize) {
            const char* partialRecord = static_cast<const char*>(record) + offset;
            char* dataBlock = Block(flag,partialRecord, recordSize);
            getPage(pageID)->AddDataBlockAt(node, dataBlock, blockSize);
            delete []dataBlock;
            return true;
        }

        if (node->offset_ != getPage(pageID)->lastOffset_) {
            return false;
        }

        if (!isAppendable) {
            return false;
        }

        // If Create New Page 
        if (IsCreateNewPage(pageID)) {
            assert(node->offset_ == getPage(pageID)->lastOffset_);
            // TODO: Compact reinsert !! 
            CreateNewPage();
            Compact(pageID);
            return RecursivelyAddRecord(node, pageID, record, offset, recordSize, 
                                        isAppendable, isFirst);
        }

        int nextPageID = pageID + 1;
        int splitSize = recordSize - BlockToDataSize(node->blockSize_);
        if (RecursivelyAddRecord(getPage(nextPageID)->freeListH_->next_, nextPageID, record,
                             offset + BlockToDataSize(node->blockSize_), splitSize, true, false)) {
                flag.flagIsAppend = true;
                const char* partialRecord = static_cast<const char*>(record) + offset;
                char* dataBlock = Block(flag, partialRecord, BlockToDataSize(node->blockSize_));
                getPage(pageID)->AddDataBlockAt(node, dataBlock, node->blockSize_);
                delete []dataBlock;
                return true;
        } else {
            return false;
        }
    }

    std::pair<char*, int> GetData(long pageID, int offset) {
        assert(pageID <= LastPageID());
        PageHeap* page = getPage(pageID);
        assert(page);
        assert(offset < page->lastOffset_);

        int totalSize = 0;
        std::vector<std::pair<PageHeap*, int>> dataBlocks;

        while (pageID <= LastPageID()) {
            PageHeap* page = getPage(pageID);
            assert(page);
            Flag flag = page->LoadFlag(offset);
            assert(flag.flagAssigned);

            int dataSize = page->LoadSize(offset);
            dataBlocks.emplace_back(page, offset);

            totalSize += dataSize;

            if (!flag.flagIsAppend) {
                break;
            }

            pageID += 1; 
            offset = 0; 
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

    int FreeBlock(PageIDType pageID, int offset) {
        PageHeap* page = getPage(pageID);
        assert(page);
        Flag flag = page->LoadFlag(offset);
        assert(flag.flagAssigned);

        if (flag.flagIsAppend) {
            FreeBlock(pageID + 1, 0);
        }

        int blockSize = BlockSize(page->LoadSize(offset));

        FreeAt(pageID,offset,blockSize);
        assert(blockSize > 0);

        return BlockToDataSize(blockSize);
    }

    void FreeAt(PageIDType pageID, OffsetType offset, int size) {
        getPage(pageID)->Free(offset, size);
        // Compact 
        if (getPage(pageID)->listSize_ == FREE_LIST_SIZE) {
            Compact(pageID);
        }
    }

    void Compact(PageIDType pageID) {
        if (getPage(pageID)->listSize_ == 1) {
            return;
        }
        PageHeap::Iterator iter = getPage(pageID)->Begin();
        OffsetType offset = 0;
        auto end = getPage(pageID)->End();
        while (iter != end) {
            PageHeap::Iterator next = iter;
            ++next;
            Flag flag = getPage(pageID)->LoadFlag(*iter);
            assert(flag.flagAssigned);

            int size = getPage(pageID)->LoadSize(*iter);
            
            Flag newflag{true, false};
            char* blockData = Block(newflag, 
                        getPage(pageID)->page_ + *iter + 6, size);
            getPage(pageID)->SetData(offset,blockData, BlockSize(size));
            offset += BlockSize(size);

            if (flag.flagIsAppend) {
                assert(pageID + 1 <= LastPageID());
                // Move next 
                int leftspace = getPage(pageID)->EndBlocks() - offset; 
                Flag nextFlag = getPage(pageID + 1)->LoadFlag(0);
                assert(nextFlag.flagAssigned);

                int nextSize = getPage(pageID + 1)->LoadSize(0);
                if (leftspace >= nextSize) {
                    // move all next block to this block
                    getPage(pageID)->SetData(offset, getPage(pageID + 1)->page_ + 6, 
                                              nextSize);
                   
                    int newSize = size + nextSize;
                    getPage(pageID)->SetData(offset - BlockSize(size) + 2,&newSize,sizeof(int)); 
                    offset += nextSize;

                    FreeAt(pageID + 1, 0, BlockSize(nextSize));
                } else {
                    // move part that fit leftspace to this block
                    getPage(pageID)->SetData(offset,getPage(pageID + 1)->page_ + 6, 
                                              leftspace);
                    int newSize = size + leftspace;
                    getPage(pageID)->SetData(offset - BlockSize(size) + 2,&newSize,sizeof(int)); 
                    bool isAppend = true;
                    getPage(pageID)->SetData(offset - BlockSize(size) + 1,&isAppend, 1); 
                    offset += leftspace;
                    assert(offset == getPage(pageID)->EndBlocks());

                    int leftSize = nextSize - leftspace;
                    // left part set at offset 0 in its page with its flag 
                    // free offset at 0 + the left part  blockSize 
    
                    getPage(pageID + 1)->SetData(0 + 6, getPage(pageID + 1)->page_ + 6 + leftspace, 
                                              leftSize);
                    getPage(pageID + 1)->SetData(0 + 2,&leftSize, sizeof(int)); 
                    getPage(pageID + 1)->SetData(0 + 1,&newflag.flagIsAppend, sizeof(bool)); 

                    FreeAt(pageID + 1, 0 + BlockSize(leftSize), nextSize - leftSize);
                    
                    // if the left part have append call compact with that page
                    if (newflag.flagIsAppend) {
                        Compact(pageID + 1);
                    }
                }
            }

            iter = next;
        }   
        // offset = lastOffset
        getPage(pageID)->lastOffset_ = offset;
        getPage(pageID)->blockSpace_ = getPage(pageID)->blockCount_;
        auto curr = getPage(pageID)->freeListH_->next_;

        // delete Free list except last one
        while (curr != getPage(pageID)->freeListH_->prev_) {
            auto next = curr->next_;
            delete curr;
            getPage(pageID)->listSize_--;
            curr = next;
        }

        assert(getPage(pageID)->listSize_ == 1); 
        curr->offset_ = offset; // lastOffset 
        curr->blockSize_ = getPage(pageID)->EndBlocks() - offset;
        curr->prev_ = getPage(pageID)->freeListH_;
        curr->next_ = getPage(pageID)->freeListH_;
        getPage(pageID)->freeListH_->next_ = curr;
        getPage(pageID)->freeListH_->prev_ = curr;
    }
    
    bool IsCreateNewPage(long pageID) {
        if (pageID == LastPageID()) {
            return true;
        }
        return false;
    }

    void CreateNewPage() {
        lastPageId_++;
        pages.push_back(PageHeap(lastPageId_, maxPageBytes_));  
    }

    long LastPageID() {
        return pages.size();
    }

    PageHeap* getPage(long pageID) {
        if (pageID <= LastPageID()) {
            return &pages[pageID - 1];  
        }
        assert(false);
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
            count += e.blockCount_;
        }
        return count;
    }

    void PrintPageInfo() {
        std::cout << "===============\n";
            for (int i = 0; i < pages.size(); i++) {
                std::cout << "Page:" << i + 1  << ", BlockCount: " << pages[i].blockCount_
                          << ", BlockSpace: " <<  pages[i].blockSpace_ << std::endl;
                pages[i].PrintFree();
            }
            std::cout << "===============\n";
    }

        size_t Size() const {
            return pages.size();
        }

        long GetNextPageId() const {
            return lastPageId_;
        }
    };
}