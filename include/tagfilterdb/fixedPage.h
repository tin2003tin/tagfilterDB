#ifndef TAGFILTERDB_PAGE_H
#define TAGFILTERDB_PAGE_H

#include "bitset.h"
#include "dataView.h"
#include "cache.h"

#include <cstring>
#include <iostream>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <map>

namespace tagfilterdb {
    class FixedPageMgr;
    constexpr size_t MINIMUM_PAGE_FILE_BYTES = 1; 

    class FixedPage {
    private:
        PageIDType pageID_;       ///< Identifier for the page.
        Bitset bitSet_;           ///< Bitmap for tracking free/used slots.
        char* page_;              ///< Pointer to the raw page data.

        int blockSize_;
        int maxBlock_;
        size_t maxPageBytes_;

        friend FixedPageMgr;
    public:
        FixedPage() = default;
        
        ~FixedPage(){
            delete []page_;
        }

        FixedPage(PageIDType pageID, size_t maxPageBytes, int blockSize_) : 
            pageID_(pageID) {

            init(maxPageBytes,blockSize_);
        }

        void init(size_t maxPageBytes, int blockSize) {
            maxPageBytes_ = maxPageBytes;
            blockSize_ = blockSize;
            setup();
        }

        void setup() {
            page_ = new char[GetMaxDataOffset()];
            maxBlock_ = maxPageBytes_ / blockSize_;
            size_t bitsetSize = (maxBlock_ + 7) / 8;
            if (maxPageBytes_ - maxBlock_ * blockSize_ < 
                sizeof(PageIDType) + sizeof(size_t) + bitsetSize) {
                    maxBlock_--;
            }
            bitSet_.Setup(maxBlock_);
        }

        // Copy Constructor
        FixedPage(const FixedPage& other) : pageID_(other.pageID_), 
                                            blockSize_(other.blockSize_), 
                                            maxBlock_(other.maxBlock_), 
                                            maxPageBytes_(other.maxPageBytes_), 
                                            bitSet_(other.bitSet_) {
            // Deep copy the page data
            page_ = new char[GetMaxDataOffset()];
            std::memcpy(page_, other.page_, GetMaxDataOffset());
        }

        // Copy Assignment Operator
        FixedPage& operator=(const FixedPage& other) {
            if (this == &other) return *this; // Handle self-assignment

            // Free existing resources
            delete[] page_;

            // Copy data
            pageID_ = other.pageID_;
            blockSize_ = other.blockSize_;
            maxBlock_ = other.maxBlock_;
            maxPageBytes_ = other.maxPageBytes_;
            bitSet_ = other.bitSet_;

            // Deep copy the page data
            page_ = new char[maxPageBytes_];
            std::memcpy(page_, other.page_, maxPageBytes_);

            return *this;
        }

        size_t getMetaDataSize() const {
           
            return  sizeof(PageIDType) // PageId 
                  + sizeof(size_t)  // bitsetSize
                  + (maxBlock_ + 7) / 8  // bitset
                  ;
        }

        /// Find the first free offset using the bitmap
        int findFreeSlot() const {
            for (size_t i = 0; i < maxBlock_; ++i) {
                if (!bitSet_.isSet(i)) {
                    return i;
                }
            }
            return -1;
        }

        /// Mark a slot as used
        void allocateSlot(size_t index) {
            if (index >= maxBlock_) {
                throw std::out_of_range("Slot index out of range.");
            }
            bitSet_.set(index);
        }

        /// Mark a slot as free
        void freeSlot(size_t index) {
            if (index >= maxBlock_) {
                throw std::out_of_range("Slot index out of range.");
            }
            bitSet_.clear(index);
        }

        /// Check if a slot is free
        bool isSlotFree(size_t index) const {
            if (index >= maxBlock_) {
                throw std::out_of_range("Slot index out of range.");
            }
            return !bitSet_.isSet(index);
        }

        /// Set data in the page at a specific offset
        void SetData(size_t offset, const char* data) {
            if (offset + blockSize_ > maxPageBytes_ - getMetaDataSize()) {
                throw std::out_of_range("Data exceeds page bounds.");
            }
            std::memcpy(page_ + offset, data, blockSize_);
        }

        /// Get data from the page at a specific offset
        void GetData(size_t offset, char* buffer) const {
            if (offset + blockSize_ > maxPageBytes_ - getMetaDataSize()) {
                throw std::out_of_range("Data exceeds page bounds.");
            }
            std::memcpy(buffer, page_ + offset, blockSize_);
        }

        char* GetBlock(size_t offset) {
            char* buffer = new char[blockSize_]; 
            std::memcpy(buffer, page_ + offset, blockSize_);
            return buffer;
        }

        char* SerializeMetaData() const {
            int metaDataSize = getMetaDataSize();
            char* buffer = new char[metaDataSize];
            size_t offset = 0;

            std::memcpy(buffer + offset, &pageID_, sizeof(pageID_));
            offset += sizeof(pageID_);

            bitSet_.Serialize(buffer, offset);
            return buffer;
        }

        void DeserializeMetaData(const char* buffer, size_t& offset) {
            std::memcpy(&pageID_, buffer + offset, sizeof(pageID_));
            offset += sizeof(pageID_);
            
            bitSet_.Deserialize(buffer, offset); 
        }

        void Load(std::istream& in) {
            size_t offset = 0;
            char metadataBuffer[getMetaDataSize()];

            in.read(metadataBuffer, getMetaDataSize());
            if (!in) {
                throw std::runtime_error("Failed to read metadata.");
            }
            DeserializeMetaData(metadataBuffer, offset);

            in.read(page_,  GetMaxDataOffset());
            if (!in) {
                throw std::runtime_error("Failed to read page data.");
            }
        }

        void Write(std::ostream& out) {
            char* metaData = SerializeMetaData();
            out.write(metaData, getMetaDataSize());
            delete []metaData;
            out.write(page_,  GetMaxDataOffset());
        }

        /// Print the bitmap for debugging
        void PrintBitmap() const {
            std::cout << "Bitmap: " << bitSet_.toString() << std::endl;
            std::cout << "Number of ones in bitmap: " << bitSet_.count() << std::endl;
        }

        OffsetType GetMaxDataOffset() {
            return maxPageBytes_ - getMetaDataSize();
        }

        /// Calculate the byte offset for a given slot index and slot size
        size_t GetSlot(size_t offset) const {
            return (offset) / blockSize_;
        }

        /// Get the page ID
        PageIDType GetPageID() const {
            return pageID_;
        }

        /// Set the page ID
        void SetPageID(PageIDType pageID) {
            pageID_ = pageID;
        }

        class Iterator {
            FixedPage* page_;
            OffsetType offset_;

        public:
            Iterator(FixedPage* page)
                : page_(page), offset_(0) {
                // skipFree();
            }

            Iterator(FixedPage* page, OffsetType offset)
                : page_(page), offset_(offset) {
            }

            void operator++() {
                assert(page_);
                
                offset_ += page_->blockSize_;
                // skipFree();
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
            // TODO:
            // void skipFree() {
            //     assert(page_);
            //     if (offset_ == page_->blockSize_ * page_->maxBlock_) {
            //         return;
            //     }

            // }
        };

        Iterator Begin() {
            return Iterator(this);
        }

        Iterator End() {
             return Iterator(this, (maxBlock_ - 1)*blockSize_);
        }
    };

    class FixedPageMgr {
    private:
        std::map<PageIDType, FixedPage> pages_;    ///< Vector to store all the pages.

        ShareLRUCache<FixedPage>* cache_;

        size_t blockSize_;
        size_t maxPageBytes_;
        size_t maxBlock_;

        PageIDType lastPageID_;

        std::string filename_;
 
    public:
       friend FixedPage;
        /// Constructor
        FixedPageMgr(std::string filename,
            size_t maxBytes, size_t blockSize, 
                ShareLRUCache<FixedPage>* cache) :
            filename_(filename), lastPageID_(0), blockSize_(blockSize),
            cache_(cache), maxPageBytes_(maxBytes) { 
            assert(cache);
            if (maxBytes < MINIMUM_PAGE_FILE_BYTES) {
                maxBytes = MINIMUM_PAGE_FILE_BYTES;
            }

            maxBlock_ = maxPageBytes_ / blockSize_;
            size_t bitsetSize = (maxBlock_ + 7) / 8;
            if (maxPageBytes_ - maxBlock_ * blockSize_ < 
                sizeof(PageIDType) + sizeof(size_t) + bitsetSize) {
                    maxBlock_--;
            }
        }

        FixedPage* getPage(PageIDType pageID) {
            if (pageID <= lastPageID_) {
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

        size_t GetMetaDataSize() {
            return sizeof(PageIDType) + // lastPageID
                   sizeof(PageIDType) + // root Page
                   sizeof(OffsetType)   // root Offset
                   ;
        }

        std::pair<FixedPage*,BaseNode*> fetchPage(PageIDType pageID) {
            // Find in cache
            auto n = cache_->Get(std::to_string(pageID));
            if (n == nullptr) {
                // If it is not found the page in cache load from disk
                FixedPage* page = LoadAtPage(pageID);
                return {page, nullptr};
            }
            return {&ShareLRUCache<FixedPage>::GetValue(n), n};
        }

        FixedPage* LoadAtPage(PageIDType pageID) {
            if (pageID < 0 || pageID > lastPageID_) {
                throw std::out_of_range("Invalid page index");
            }
            long pageIndex = pageID - 1;

            std::ifstream in(filename_, std::ios::in | std::ios::out | std::ios::binary);
            if (!in) {
                throw std::runtime_error("Failed to open file for loading.");
            }

            size_t pageOffset = GetMetaDataSize() + (pageIndex * (maxPageBytes_));
            in.seekg(pageOffset, std::ios::beg);
            if (!in) {
                throw std::runtime_error("Failed to seek to the specific page in the file.");
            }
                FixedPage* loadedPage = new FixedPage();
                loadedPage->init(maxPageBytes_, blockSize_);
                try {
                    loadedPage->Load(in);
                } catch (const std::exception& e) {
                    throw std::runtime_error("Failed to load the specified page: " + std::string(e.what()));
                }

                in.close();

                return loadedPage;
        }

        void CreateNewPage() {
            lastPageID_++;
            pages_[lastPageID_].maxPageBytes_ = maxPageBytes_;
            pages_[lastPageID_].blockSize_ = blockSize_;
            pages_[lastPageID_].setup();
            pages_[lastPageID_].pageID_ = lastPageID_;
        }

        std::pair<FixedPage*,int> AllocatePage(PageIDType pageID) {
            while (true) {
                FixedPage* page = getPage(pageID);
                int slot = page->findFreeSlot();
                if (slot != -1) {
                    return {page,slot};
                }
                pageID++;
            }
        }

        BlockAddress Assign(PageIDType pageID) {
            auto res = AllocatePage(pageID);  

            res.first->allocateSlot(res.second);       
            OffsetType offset = res.second * blockSize_;

            return {res.first->GetPageID(), offset};  
        }

        DataView FetchData(BlockAddress addr) {
            assert(addr.pageID <= LastPageID());

            auto res = fetchPage(addr.pageID);
            assert(res.first);

            char* buffer = new char[blockSize_];
            int writeOffset = 0;

            res.first->GetData(addr.offset, buffer);
            HandleCache(res.first, res.second);
  

            return {buffer, blockSize_};
        }

        void HandleCache(FixedPage* page, BaseNode* cacheEntry) {
            if (cacheEntry == nullptr) {
                cache_->Release(cache_->Insert(
                    std::to_string(page->pageID_), *page, maxPageBytes_));
                delete page;
            } else {
                cache_->Release(cacheEntry);
            }
        }

        /// Print information about all pages
        void PrintPageInfo() const {
            for (const auto& e : pages_) {
                std::cout << "Page ID: " << e.second.GetPageID() << std::endl;
                e.second.PrintBitmap();
            }
        }

        /// Get the next available page ID
        PageIDType LastPageID() const {
            return lastPageID_;
        }

        SignableData Load() {
            std::ifstream in(filename_, std::ios::binary);
            if (!in) {
                std::ofstream createFile(filename_, std::ios::binary);
                if (!createFile) {
                    throw std::runtime_error("Failed to create missing file.");
                }
                createFile.close();
                std::cout << "File not found. Created: " << filename_ << std::endl;
                return SignableData();
            }

            in.read(reinterpret_cast<char*>(&lastPageID_), sizeof(PageIDType));

            BlockAddress rootAddr;
            in.read(reinterpret_cast<char*>(&rootAddr.pageID), sizeof(PageIDType));
            in.read(reinterpret_cast<char*>(&rootAddr.offset), sizeof(OffsetType));

            auto res = fetchPage(rootAddr.pageID);
            DataView view;
            view.data = res.first->GetBlock(rootAddr.offset);
            view.size = blockSize_;

            HandleCache(res.first,res.second);

            if (!in) {
                throw std::runtime_error("Failed to read lastPageId from file.");
            }
            in.close();

            return SignableData(view, rootAddr);
        }

        void Flush(BlockAddress rootAddr) {
            std::ofstream out(filename_, std::ios::binary | std::ios::in | std::ios::out);
            if (!out) {
                out.open(filename_, std::ios::binary | std::ios::trunc | std::ios::out);
                if (!out) {
                    throw std::runtime_error("Failed to create a new file.");
                }
            }

            out.seekp(0, std::ios::beg);
            size_t managerSize = sizeof(PageIDType) + sizeof(PageIDType) + sizeof(OffsetType);
            out.write(reinterpret_cast<const char*>(&lastPageID_), sizeof(PageIDType));
            out.write(reinterpret_cast<const char*>(&rootAddr.pageID), sizeof(PageIDType));
            out.write(reinterpret_cast<const char*>(&rootAddr.offset), sizeof(OffsetType));

            if (!out) {
                throw std::runtime_error("Failed to write lastPageID_ to file.");
            }

            for (auto& [pageID, page] : pages_) {
                out.seekp((pageID - 1) * maxPageBytes_ + managerSize, std::ios::beg);
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
} // namespace tagfilterdb

#endif // TAGFILTERDB_PAGE_H
