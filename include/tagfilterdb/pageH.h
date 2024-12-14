#pragma once

#include "bitset.h"
#include <cstring>
#include <iostream>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <functional>
#include <fstream>
#include <map>
#include "tagfilterdb/cache.h"
#include "tagfilterdb/dataView.h"
#include "tagfilterdb/list.h"

#include "json.hpp"

using namespace nlohmann;

namespace tagfilterdb {
    class PageHeapManager;
    constexpr size_t MINIMUM_FILE_BYTES = 1; 
    constexpr int MIN_SIZE = 2 + 4;
    constexpr int FREE_LIST_SIZE = 10;
    constexpr int MAXIMUM_FILE_BYTES = 1024 * 4;
    constexpr long MAXINUM_CACHE_CHARGE = 1024*4* 100; // can cache 100 Page !!  

    // [TODO] make env store file to disk
    const std::string PAGEHEAP_FILENAME = "fileHeap.txt";
    
    #pragma pack(1)
    struct Flag {
        bool flagAssigned;
        bool flagIsAppend;
    };
    #pragma pack()

    // [MARK] clean the pageHeap class structure
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

      // The Iterator that traval all assigned block in the page 
      class Iterator {
            PageHeap* page_;
            OffsetType offset_;

        public:
            Iterator(PageHeap* page)
                : page_(page), offset_(0) {
                skipFree();
            }

            Iterator(PageHeap* page, OffsetType offset)
                : page_(page), offset_(offset) {
            }

            void operator++() {
                assert(page_);
                
                offset_ = page_->nextOffset(offset_);
                skipFree();
            }

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
            return sizeof(PageIDType)                                     // for PageIDType
                + sizeof(listSize_)                                       // for ListSize
                + (sizeof(OffsetType) + sizeof(int)) *FREE_LIST_SIZE      // for offsets of free space (worst case is half of the data)
                + sizeof(OffsetType)                                      // for lastOffset
                + sizeof(blockSpace_)                                     // for blockSpace
                + sizeof(blockCount_)                                     // for blockCount 
                ; 
        }

        PageHeap() = default;

        PageHeap(PageIDType pageId, size_t maxPageBytes) : 
            PageId_(pageId), 
            maxPageBytes_(maxPageBytes), blockSpace_(0),
            listSize_(0), blockCount_(0),
            lastOffset_(0) {

            setup();
            initFreeList();
        }

          void setup() {
            page_ = new char[EndBlocks()];
            freeListH_ = new FreeList(0,0, nullptr, nullptr);
            freeListH_->next_ = freeListH_;
            freeListH_->prev_ = freeListH_;
        }

        void initFreeList() {
            FreeList* firstList = new FreeList(0, maxPageBytes_ - getMetaDataSize(),
                                                freeListH_,freeListH_);

            freeListH_->next_ = firstList;
            freeListH_->prev_ = firstList;
            listSize_++;
        }

         ~PageHeap() {
            delete []page_;
            if (freeListH_ == nullptr) {
                return;
            }
            auto curr = freeListH_->next_;
            
            while (curr != freeListH_) {
                auto next = curr->next_;
                delete curr;
                curr = next;
            }
            delete freeListH_;
        }

        PageHeap(const PageHeap& other)
            : PageId_(other.PageId_),
            maxPageBytes_(other.maxPageBytes_),
            blockSpace_(other.blockSpace_),
            listSize_(other.listSize_),
            blockCount_(other.blockCount_),
            lastOffset_(other.lastOffset_) {
            // Allocate new memory for the page
            page_ = new char[maxPageBytes_];
            std::copy(other.page_, other.page_ + maxPageBytes_, page_);
            
            // Deep copy the free list
            if (other.freeListH_) {
                freeListH_ = new FreeList(0, 0, nullptr, nullptr);
                freeListH_->next_ = freeListH_;
                freeListH_->prev_ = freeListH_;

                auto currOther = other.freeListH_->next_;
                auto currThis = freeListH_;

                while (currOther != other.freeListH_) {
                    auto newNode = new FreeList(currOther->offset_, currOther->blockSize_, freeListH_, currThis);
                    currThis->next_ = newNode;
                    currOther = currOther->next_;
                    currThis = newNode;
                }
            }
        }

        PageHeap& operator=(const PageHeap& other) {
            if (this == &other) return *this; // Self-assignment check

            // Clean up existing resources
            delete[] page_;
            this->~PageHeap();

            // Copy data
            PageId_ = other.PageId_;
            maxPageBytes_ = other.maxPageBytes_;
            blockSpace_ = other.blockSpace_;
            listSize_ = other.listSize_;
            blockCount_ = other.blockCount_;
            lastOffset_ = other.lastOffset_;

            page_ = new char[maxPageBytes_];
            std::copy(other.page_, other.page_ + maxPageBytes_, page_);

            // Deep copy the free list
            if (other.freeListH_) {
                freeListH_ = new FreeList(0, 0, nullptr, nullptr);
                freeListH_->next_ = freeListH_;
                freeListH_->prev_ = freeListH_;

                auto currOther = other.freeListH_->next_;
                auto currThis = freeListH_;

                while (currOther != other.freeListH_) {
                    auto newNode = new FreeList(currOther->offset_, currOther->blockSize_, freeListH_, currThis);
                    currThis->next_ = newNode;
                    currOther = currOther->next_;
                    currThis = newNode;
                }

                freeListH_->prev_ = currThis;
            }

            return *this;
        }

        PageHeap(PageHeap&& other) noexcept
            : PageId_(other.PageId_),
            page_(other.page_),
            freeListH_(other.freeListH_),
            listSize_(other.listSize_),
            maxPageBytes_(other.maxPageBytes_),
            lastOffset_(other.lastOffset_),
            blockSpace_(other.blockSpace_),
            blockCount_(other.blockCount_) {
            // Nullify the source
            other.page_ = nullptr;
            other.freeListH_ = nullptr;
        }

        PageHeap& operator=(PageHeap&& other) noexcept {
            if (this == &other) return *this; // Self-assignment check

            // Clean up existing resources
            delete[] page_;
            this->~PageHeap();

            // Steal data
            PageId_ = other.PageId_;
            page_ = other.page_;
            freeListH_ = other.freeListH_;
            listSize_ = other.listSize_;
            maxPageBytes_ = other.maxPageBytes_;
            lastOffset_ = other.lastOffset_;
            blockSpace_ = other.blockSpace_;
            blockCount_ = other.blockCount_;

            // Nullify the source
            other.page_ = nullptr;
            other.freeListH_ = nullptr;

            return *this;
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

    char* SerializeMetaData() {
        // Compute metadata size and allocate memory
        int metaDataSize = getMetaDataSize();
        char* metaData = new char[metaDataSize];
        int offset = 0;

        // Serialize fixed-size metadata
        std::memcpy(metaData + offset, &PageId_, sizeof(PageIDType));
        offset += sizeof(PageIDType);
        std::memcpy(metaData + offset, &lastOffset_, sizeof(int));
        offset += sizeof(int);
        std::memcpy(metaData + offset, &blockSpace_, sizeof(int));
        offset += sizeof(int);
        std::memcpy(metaData + offset, &blockCount_, sizeof(int));
        offset += sizeof(int);
        std::memcpy(metaData + offset, &listSize_, sizeof(int));
        offset += sizeof(int);

        // Serialize free list metadata
        FreeList* curr = freeListH_->next_;
        while (curr != freeListH_) {
            if (offset + sizeof(OffsetType) + sizeof(int) > metaDataSize) {
                delete[] metaData;
                throw std::runtime_error("Metadata size calculation error");
            }

            std::memcpy(metaData + offset, &curr->offset_, sizeof(OffsetType));
            offset += sizeof(OffsetType);
            std::memcpy(metaData + offset, &curr->blockSize_, sizeof(int));
            offset += sizeof(int);
            curr = curr->next_;
        }

        // Sanity check: Ensure offset matches metaDataSize
        if (offset + 8 * (FREE_LIST_SIZE - listSize_) != metaDataSize) {
            delete[] metaData;
            throw std::runtime_error("Mismatch between metadata size and serialization offset");
        }

        return metaData;
    }


    void Write(std::ostream& out) {
        char* metaData = SerializeMetaData();
        out.write(metaData, getMetaDataSize());
        delete []metaData;
        out.write(page_, EndBlocks());
    }

    void LoadMetadataOnly(std::istream& in) {
         // Allocate memory for metadata
        char* metaData = new char[getMetaDataSize()];
        in.read(metaData, getMetaDataSize());
        if (!in) {
            delete[] metaData;
            throw std::runtime_error("Failed to read page metadata.");
        }

        int offset = 0;

        std::memcpy(&PageId_, metaData + offset, sizeof(PageIDType));
        offset += sizeof(PageIDType);

        std::memcpy(&lastOffset_, metaData + offset, sizeof(int));
        offset += sizeof(int);

        std::memcpy(&blockSpace_, metaData + offset, sizeof(int));
        offset += sizeof(int);

        std::memcpy(&blockCount_, metaData + offset, sizeof(int));
        offset += sizeof(int);

        std::memcpy(&listSize_, metaData + offset, sizeof(int));
        offset += sizeof(int);

        FreeList* prev = freeListH_;
        for (int i = 0; i < listSize_; ++i) {
            OffsetType offsetVal;
            int blockSize;

            std::memcpy(&offsetVal, metaData + offset, sizeof(OffsetType));
            offset += sizeof(OffsetType);

            std::memcpy(&blockSize, metaData + offset, sizeof(int));
            offset += sizeof(int);

            FreeList* newNode = new FreeList(offsetVal, blockSize, nullptr, nullptr);
            prev->next_ = newNode;
            newNode->prev_ = prev;
            prev = newNode;
        }

        prev->next_ = freeListH_;
        freeListH_->prev_ = prev;
        delete[] metaData;
    }

    void Load(std::istream& in) {
        LoadMetadataOnly(in);
        in.read(page_, EndBlocks());
        if (!in) {
            throw std::runtime_error("Failed to read page data.");
        }
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
        std::map<int, PageHeap> pages_;    ///< Vector to store all the pages.

        ShareLRUCache<PageHeap>* cache_;

        size_t maxPageBytes_;
        
        PageIDType lastPageId_;           ///< Page ID for the next page to be allocated.

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
                        if (pageID_ > pm_->LastPageID()) {
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
        PageHeapManager(size_t maxBytes, ShareLRUCache<PageHeap>* cache) 
            : lastPageId_(0), cache_(cache), maxPageBytes_(maxBytes)
        {
            assert(cache);
            if (maxBytes < MINIMUM_FILE_BYTES) {
                maxBytes = MINIMUM_FILE_BYTES;
            }
        }

        ~PageHeapManager() {}

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

    BlockAddress AddRecord(const char* record, int recordSize,
                           List<AdjustData>* clist) {
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
            if (RecursivelyAddRecord(node,pageID,record,0,recordSize, 
                                    isAppendable, true, clist)) {
                break;
            }
            // Create New page ?
            if (IsCreateNewPage(pageID)) {
                Compact(pageID, clist);
                CreateNewPage();
            }   
            pageID++;
        }
        assert(assignedOffset >= 0);
        return {pageID, assignedOffset};
    }

    bool RecursivelyAddRecord(PageHeap::FreeList* node, long pageID, const void* record, 
                              OffsetType offset, int recordSize, bool isAppendable, 
                              bool isFirst, List<AdjustData>* clist) {
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
            CreateNewPage();
            Compact(pageID, clist);
            return RecursivelyAddRecord(node, pageID, record, offset, recordSize, 
                                        isAppendable, isFirst, clist);
        }

        int nextPageID = pageID + 1;
        int splitSize = recordSize - BlockToDataSize(node->blockSize_);
        if (RecursivelyAddRecord(getPage(nextPageID)->freeListH_->next_, nextPageID, record,
                             offset + BlockToDataSize(node->blockSize_), splitSize, 
                            true, false, clist)) {
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

    DataView GetData(BlockAddress addr) {
        assert(addr.pageID <= LastPageID());
        PageHeap* page = getPage(addr.pageID);
        assert(page);
        assert(addr.offset < page->lastOffset_);

        size_t totalSize = 0;
        std::vector<std::pair<PageHeap*, int>> dataBlocks;

        while (addr.pageID <= LastPageID()) {
            PageHeap* page = getPage(addr.pageID);
            assert(page);
            Flag flag = page->LoadFlag(addr.offset );
            assert(flag.flagAssigned);

            int dataSize = page->LoadSize(addr.offset );
            dataBlocks.emplace_back(page, addr.offset );

            totalSize += dataSize;

            if (!flag.flagIsAppend) {
                break;
            }

            addr.pageID += 1; 
            addr.offset = 0; 
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

    bool FreeBlock(PageIDType pageID, int offset, bool isStress, List<AdjustData>* clist ) {
        PageHeap* page = getPage(pageID);
        assert(page);
        Flag flag = page->LoadFlag(offset);
        assert(flag.flagAssigned);

        if (flag.flagIsAppend) {
            FreeBlock(pageID + 1, 0, isStress, clist);
        }

        int blockSize = BlockSize(page->LoadSize(offset));

        return FreeAt(pageID,offset,blockSize,isStress, clist);
    }
    
    bool FreeAt(PageIDType pageID, OffsetType offset, 
                int size, bool isStress, List<AdjustData>* clist) {
        getPage(pageID)->Free(offset, size);
        if (isStress && MayCompact(pageID)) {
            return Compact(pageID,clist);
        }
        return false;
    }

    bool MayCompact(PageIDType pageID) {
         if (getPage(pageID)->listSize_ >= FREE_LIST_SIZE) {
            return true;
        } else {
            return false;
        }
    }

    bool Compact(PageIDType pageID, List<AdjustData>* clist) {
        struct Adjust {
            BlockAddress oldAddr;
            BlockAddress newAddr;
        };
        std::vector<Adjust> adjustVec;
        bool isStop = false;
        bool isAppend = false;
        while (!isStop && pageID <= lastPageId_) {
            if (getPage(pageID)->listSize_ == 1) {
                return false;
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
                // check is it same offset  
                if (offset != *iter) {
                    char* blockData = Block(newflag, 
                            getPage(pageID)->page_ + *iter + 6, size);
                    getPage(pageID)->SetData(offset,blockData, BlockSize(size));
                    delete []blockData;
                    if (!isAppend) {
                        adjustVec.push_back(
                                {BlockAddress{pageID,*iter}, 
                                 BlockAddress{pageID,offset}});
                    }
                }
            
                offset += BlockSize(size);

                if (flag.flagIsAppend) {
                    isAppend = true;
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

                        FreeAt(pageID + 1, 0, BlockSize(nextSize), true, clist);
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

                        FreeAt(pageID + 1, 0 + BlockSize(leftSize), nextSize - leftSize, true, clist);
                        
                        // if the left part have append call compact with that page
                        if (newflag.flagIsAppend) {
                            pageID++;
                        } 
                    } 
                } else {
                    isStop = true;
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

        // Load data and add to compact List
        for (auto e : adjustVec) {
            DataView loadedData = GetData(e.newAddr);
            clist->Add(AdjustData{loadedData,e.oldAddr,e.newAddr});
        }

        return true;
    }
    
    bool IsCreateNewPage(long pageID) {
        if (pageID == LastPageID()) {
            return true;
        }
        return false;
    }

    void CreateNewPage() {
        lastPageId_++;
        pages_[lastPageId_].maxPageBytes_ = maxPageBytes_;
        pages_[lastPageId_].setup();
        pages_[lastPageId_].initFreeList();
        pages_[lastPageId_].PageId_ = lastPageId_;
    }

    long LastPageID() {
        return lastPageId_;
    }

    // This is used for assign/free/flush/compact process ??private
    PageHeap* getPage(long pageID) {
        if (pageID <= LastPageID()) {
            // Find the page in pages 
            if (pages_.count(pageID)) {
                return &pages_[pageID];
            }  else {
                // Fetch the page form cache/disk
                auto res = fetchPage(pageID);
                // Copy the page to pages 
                pages_[pageID] = *(res.first);
                cache_->Release(res.second);
                return &pages_[pageID];
            }
        } else { 
            // Create new page if request more than lastPage
           CreateNewPage();
        }
        
        return &pages_[pageID];
    }

    // This is more general for external can request data
     std::pair<PageHeap*,BaseNode*> fetchPage(long pageID) {
        // Find in cache
        auto n = cache_->Get(std::to_string(pageID));
        if (n == nullptr) {
            // If it is not found the page in cache load from disk
            PageHeap* page = LoadAtPage(pageID);
            return {page, nullptr};
        }
        return {&ShareLRUCache<PageHeap>::GetValue(n), n};
    }

    Iterator begin() {
        return Iterator(this);
    }

    Iterator end() {
        int lastOffset = getPage(lastPageId_)->lastOffset_;
        Iterator iter(this,lastPageId_,lastOffset);
        return iter;
    }

    int TotalCount() {
        int count = 0;
        for (int i = 0; i < lastPageId_; i++) {
                count += getPage(i + 1)->blockCount_;
        }    
        return count;
    }

    void PrintPageInfo() {
        std::cout << "===============\n";
            for (int i = 0; i < lastPageId_; i++) {
                std::cout << "Page:" << i + 1  << ", BlockCount: " << pages_[i].blockCount_
                          << ", BlockSpace: " <<  getPage(i +1)->blockSpace_ << std::endl;
                getPage(i +1)->PrintFree();
            }
            std::cout << "===============\n";
    }

    size_t Size() const {
        return lastPageId_;
    }

    long GetNextPageId() const {
        return lastPageId_;
    }

    void Save() {
        std::ofstream out(PAGEHEAP_FILENAME, std::ios::in | std::ios::out | std::ios::trunc);
        if (!out) {
            throw std::runtime_error("Failed to open file for saving.");
        }
        char lastPageId[sizeof(PageIDType)];
        std::memcpy(lastPageId,&lastPageId_,sizeof(PageIDType));
        out.write(lastPageId, sizeof(PageIDType));
        for (int i = 1; i <= lastPageId_; i++) {
            getPage(i)->Write(out);
        }
        out.close();
    }

    // Load Meta of Page Manager
    void Load() {
        std::ifstream in(PAGEHEAP_FILENAME, std::ios::binary);
        if (!in) {
            throw std::runtime_error("Failed to open file for loading.");
        }

        in.read(reinterpret_cast<char*>(&lastPageId_), sizeof(PageIDType));
        if (!in) {
            throw std::runtime_error("Failed to read lastPageId from file.");
        }
        in.close();
    }

    // This is just funtion for checking the test
    void LoadAllPage() {
        std::ifstream in(PAGEHEAP_FILENAME, std::ios::binary);
        if (!in) {
            throw std::runtime_error("Failed to open file for loading pages.");
        }

        in.seekg(sizeof(int), std::ios::beg);

        while (in.peek() != EOF) {
            PageHeap newPage;
            newPage.setup();
            try {
                newPage.Load(in); 
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to load page data: " + std::string(e.what()));
            }
            pages_[newPage.PageId_] = std::move(newPage);
        }
        in.close();
    }

    // This is used to load the page from disk
    PageHeap* LoadAtPage(PageIDType pageID) {
    if (pageID < 0 || pageID > lastPageId_) {
        throw std::out_of_range("Invalid page index");
    }
    long pageIndex = pageID - 1;

    std::ifstream in(PAGEHEAP_FILENAME, std::ios::in | std::ios::out | std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open file for loading.");
    }

    size_t pageOffset = sizeof(PageIDType) + (pageIndex * (maxPageBytes_));
    in.seekg(pageOffset, std::ios::beg);
    if (!in) {
        throw std::runtime_error("Failed to seek to the specific page in the file.");
    }

        PageHeap* loadedPage = new PageHeap();
        loadedPage->maxPageBytes_ = maxPageBytes_;
        loadedPage->setup();
        try {
            loadedPage->Load(in);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to load the specified page: " + std::string(e.what()));
        }

        in.close();

        return loadedPage;
    }

    // [OPTION]
    PageHeap LoadPageMedata(PageIDType pageID) {
    if (pageID < 0 || pageID > lastPageId_) {
        throw std::out_of_range("Invalid page index");
    }
    long pageIndex = pageID - 1;

    std::ifstream in(PAGEHEAP_FILENAME, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open file for loading.");
    }

    size_t pageOffset = sizeof(int) + (pageIndex * (maxPageBytes_));
    in.seekg(pageOffset, std::ios::beg);
    if (!in) {
        throw std::runtime_error("Failed to seek to the specific page in the file.");
    }

        PageHeap newPage;
        try {
            newPage.LoadMetadataOnly(in);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to load the specified page: " + std::string(e.what()));
        }

        in.close();
    }

    DataView FetchData(BlockAddress addr) {
        assert(addr.pageID <= LastPageID());

        size_t totalSize = 0;
        struct Temp {
            PageHeap* page;
            OffsetType offset;
            size_t dataSize;
            BaseNode* ref;
        };
        std::vector<Temp> tempVec;

        while (addr.pageID <= LastPageID()) {
            auto res = fetchPage(addr.pageID);
            assert(res.first);
            Flag flag = res.first->LoadFlag(addr.offset);
            assert(flag.flagAssigned);

            size_t dataSize = res.first->LoadSize(addr.offset);
            tempVec.push_back(Temp{res.first, addr.offset, dataSize, res.second});

            totalSize += dataSize;

            if (!flag.flagIsAppend) {
                break;
            }

            addr.pageID += 1; 
            addr.offset = 0; 
        }

        // Allocate buffer for the entire data
        char* buffer = new char[totalSize];
        int writeOffset = 0;

        // Load data into the buffer
        for (const auto& e : tempVec) {
            e.page->LoadData(e.offset, buffer + writeOffset, e.dataSize);
            writeOffset += e.dataSize;
            if (e.ref == nullptr) {
                cache_->Release(cache_->Insert(std::to_string(e.page->PageId_), 
                                std::move(*e.page), maxPageBytes_));
                delete e.page;
            } else {
                cache_->Release(e.ref);
            }
        }

        return {buffer, totalSize};
    }


   void Flush() {
        std::ofstream out(PAGEHEAP_FILENAME, std::ios::binary | std::ios::in | std::ios::out);
        if (!out) {
            throw std::runtime_error("Failed to open file for updating.");
        }

        out.seekp(0, std::ios::beg); 
        out.write(reinterpret_cast<const char*>(&lastPageId_), sizeof(PageIDType));
        if (!out) {
            throw std::runtime_error("Failed to write lastPageId_ to file.");
        }

        for (auto& [pageID, page] : pages_) {
            out.seekp((pageID - 1) * maxPageBytes_ + sizeof(PageIDType), std::ios::beg);
            if (!out) {
                throw std::runtime_error("Failed to seek to the correct position for page data.");
            }

            getPage(pageID)->Write(out);

            if (!out) {
                throw std::runtime_error("Failed to write page data to file.");
            }
        }

        out.close();
        if (!out) {
            throw std::runtime_error("Failed to properly close the file.");
        }
    }

    };
}