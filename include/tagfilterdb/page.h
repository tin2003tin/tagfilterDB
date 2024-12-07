#ifndef TAGFILTERDB_PAGE_H
#define TAGFILTERDB_PAGE_H

#include "bitset.h"
#include <cstring>
#include <iostream>
#include <cstdint>
#include <stdexcept>

namespace tagfilterdb {
    class PageNodeManager;
    constexpr size_t MINIMUM_FILE_BYTES = 1; 

    using PageIDType = long;
    using CheckSumType = uint32_t;

    class Page {
    private:
        PageIDType PageId_;       ///< Identifier for the page.
        Bitset bitSet_;           ///< Bitmap for tracking free/used slots.
        char* page_;              ///< Pointer to the raw page data.

        CheckSumType checksum_;      ///< Checksum for verifying page integrity.

        size_t nodeSize_;
        size_t maxNode_;
        size_t maxPageBytes_;

        friend PageNodeManager;
    public:
        /// Constructor
        Page(long pageId, size_t maxPageBytes, size_t nodeSize_) : 
            PageId_(pageId), checksum_(0), 
            maxPageBytes_(maxPageBytes), nodeSize_(nodeSize_) {
            page_ = new char[maxPageBytes];
            std::memset(page_, 0, maxPageBytes); 

            setup();
        }

        void setup() {
            size_t pageIDSize = sizeof(PageIDType);
            size_t CheckSumSize = sizeof(CheckSumType);
            maxNode_ = maxPageBytes_ / nodeSize_;
            size_t bitsetSize = (maxNode_ + 7) / 8;
            if (maxPageBytes_ - maxNode_ * nodeSize_ < 
                pageIDSize + CheckSumSize + bitsetSize) {
                    maxNode_--;
            }
            bitSet_.Setup(maxNode_);
        }

        size_t getMetaDataSize() const {
           
            return  sizeof(PageIDType) + sizeof(CheckSumType) + (maxNode_ + 7) / 8;
        }

        /// Find the first free offset using the bitmap
        int findFreeSlot() const {
            for (size_t i = 0; i < maxNode_ && i < (maxPageBytes_ - getMetaDataSize()) / nodeSize_; ++i) {
                if (!bitSet_.isSet(i)) {
                    return i;
                }
            }
            return -1;
        }

        /// Mark a slot as used
        void allocateSlot(size_t index) {
            if (index >= maxNode_) {
                throw std::out_of_range("Slot index out of range.");
            }
            bitSet_.set(index);
        }

        /// Mark a slot as free
        void freeSlot(size_t index) {
            if (index >= maxNode_) {
                throw std::out_of_range("Slot index out of range.");
            }
            bitSet_.clear(index);
        }

        /// Check if a slot is free
        bool isSlotFree(size_t index) const {
            if (index >= maxNode_) {
                throw std::out_of_range("Slot index out of range.");
            }
            return !bitSet_.isSet(index);
        }

        /// Set data in the page at a specific offset
        void SetData(size_t offset, const char* data) {
            if (offset + nodeSize_ > maxPageBytes_ - getMetaDataSize()) {
                throw std::out_of_range("Data exceeds page bounds.");
            }
            std::memcpy(page_ + offset, data, nodeSize_);
            UpdateChecksum(); ///< Recompute checksum after modifying data.
        }

        /// Get data from the page at a specific offset
        void GetData(size_t offset, char* buffer) const {
            if (offset + nodeSize_ > maxPageBytes_ - getMetaDataSize()) {
                throw std::out_of_range("Data exceeds page bounds.");
            }
            std::memcpy(buffer, page_ + offset, nodeSize_);
        }

        char* GetData(size_t offset) {
            return page_ + offset;
        }

        /// Compute the checksum for the page
        uint32_t ComputeChecksum() const {
            std::hash<std::string> hasher;  // Use std::string instead of std::string_view
            return static_cast<uint32_t>(hasher(std::string(page_, maxPageBytes_ - getMetaDataSize())));  // Convert to std::string
        }

        /// Update the checksum
        void UpdateChecksum() {
            checksum_ = ComputeChecksum();
        }

        /// Validate the checksum
        bool ValidateChecksum() const {
            return checksum_ == ComputeChecksum();
        }

        /// Print the bitmap for debugging
        void PrintBitmap() const {
            std::cout << "Bitmap: " << bitSet_.toString() << std::endl;
            std::cout << "Number of ones in bitmap: " << bitSet_.count() << std::endl;
        }

        /// Calculate the byte offset for a given slot index and slot size
        size_t GetSlot(size_t offset) const {
            return (offset) / nodeSize_;
        }

        /// Get the page ID
        long GetPageId() const {
            return PageId_;
        }

        /// Set the page ID
        void SetPageId(long pageId) {
            PageId_ = pageId;
        }

        /// Get the checksum
        uint32_t GetChecksum() const {
            return checksum_;
        }
    };

    class PageNodeManager {
    private:
        std::vector<Page> pages;    ///< Vector to store all the pages.

        size_t nodeSize_;
        size_t maxPageBytes_;
        
        long nextPageId_;           ///< Page ID for the next page to be allocated.
    friend Page;
    public:
        /// Constructor
        PageNodeManager(size_t maxBytes, size_t nodeSize) : nextPageId_(1), nodeSize_(nodeSize) {  // Start with page ID 1.
            if (maxBytes < MINIMUM_FILE_BYTES) {
                maxBytes = MINIMUM_FILE_BYTES;
            }
            maxPageBytes_ = maxBytes;

            pages.push_back(Page(nextPageId_, maxPageBytes_, nodeSize_));  // Initialize the first page.
        }

        ~PageNodeManager() {
            for (auto& p : pages) {
                delete []p.page_;
                delete []p.bitSet_.data_;
            }
        }

        /// Allocate a page
        Page* AllocatePage(long pageID) {
            // If the requested pageID is larger than the current pages, we must add a new page.
            if (pageID > pages.size()) {
                pages.push_back(Page(pageID, maxPageBytes_, nodeSize_));  // Create a new page
                nextPageId_ = pageID + 1;  // Update the nextPageId_ for future allocations
            }

            // Loop through the pages to find the first available page with free slots
            while (pageID <= pages.size()) {
                Page& targetPage = pages[pageID - 1];
                if (targetPage.findFreeSlot() != -1) { // Check if there's a free slot
                    return &targetPage;
                }
                // If no free slots, increment the pageID
                pageID++;
            }

            // If no available page found, create a new page and update nextPageId_
            pages.push_back(Page(++nextPageId_, maxPageBytes_, nodeSize_));
            return &pages.back();
        }

        std::pair<long, int> Assign(long pageID) {
            Page* page = AllocatePage(pageID);  

            int slot = page->findFreeSlot();    
            page->allocateSlot(slot);       
            int offset = slot * nodeSize_;

            return {page->GetPageId(), offset};  
        }

        Page* getPage(long pageID) {
            if (pageID <= pages.size()) {
                return &pages[pageID - 1];  
            }
            return nullptr;
        }

        /// Print information about all pages
        void PrintPageInfo() const {
            for (const auto& page : pages) {
                std::cout << "Page ID: " << page.GetPageId() << ", Checksum: " << page.GetChecksum() << std::endl;
                page.PrintBitmap();
            }
        }

        size_t Size() const {
            return pages.size();
        }

        /// Get the next available page ID
        long GetNextPageId() const {
            return nextPageId_;
        }
    };

} // namespace tagfilterdb

#endif // TAGFILTERDB_PAGE_H
