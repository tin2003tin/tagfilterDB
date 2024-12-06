#ifndef TAGFILTERDB_PAGE_H
#define TAGFILTERDB_PAGE_H

#include "bitset.h"
#include <cstring>
#include <iostream>
#include <cstdint>
#include <stdexcept>

namespace tagfilterdb {
    class PageManager;
    constexpr size_t MINIMUM_FILE_BYTES = 1; 

    using PageIDType = long;
    using CheckSumType = uint32_t;

    class Page {
    private:
        PageIDType PageId_;            ///< Identifier for the page.
        Bitset bitSet_;  ///< Bitmap for tracking free/used slots.
        char* page_;              ///< Pointer to the raw page data.

        CheckSumType checksum_;      ///< Checksum for verifying page integrity.

        size_t maxBytes_;
        size_t maxNode_;

        friend PageManager;
    public:
        /// Constructor
        Page(long pageId, size_t maxBytes, size_t maxNode) : PageId_(pageId), checksum_(0), 
            maxBytes_(maxBytes), maxNode_(maxNode), bitSet_(maxNode)  {
            page_ = new char[maxBytes_];
            std::memset(page_, 0, maxBytes_); ///< Initialize all bytes to zero.
        }

        size_t getMetaDataSize() const {
            return sizeof(PageIDType) + sizeof(CheckSumType) + (maxNode_ + 7) / 8;
        }

        /// Find the first free offset using the bitmap
        int findFreeSlot(size_t size) const {
            for (size_t i = 0; i < maxNode_ && i < (maxBytes_ - getMetaDataSize()) / size; ++i) {
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
        void SetData(size_t offset, const char* data, size_t size) {
            if (offset + size > maxBytes_ - getMetaDataSize()) {
                throw std::out_of_range("Data exceeds page bounds.");
            }
            std::memcpy(page_ + offset, data, size);
            UpdateChecksum(); ///< Recompute checksum after modifying data.
        }

        /// Get data from the page at a specific offset
        void GetData(size_t offset, char* buffer, size_t size) const {
            if (offset + size > maxBytes_ - getMetaDataSize()) {
                throw std::out_of_range("Data exceeds page bounds.");
            }
            std::memcpy(buffer, page_ + offset, size);
        }

        char* GetData(size_t offset) {
            return page_ + offset;
        }

        /// Compute the checksum for the page
        uint32_t ComputeChecksum() const {
            std::hash<std::string> hasher;  // Use std::string instead of std::string_view
            return static_cast<uint32_t>(hasher(std::string(page_, maxBytes_ - getMetaDataSize())));  // Convert to std::string
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
        size_t GetOffset(size_t slot, size_t slotSize) const {
            return (slot) * slotSize;
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

    class PageManager {
    private:
        std::vector<Page> pages;    ///< Vector to store all the pages.
        size_t maxBytes_;
        size_t maxNode_;

        long nextPageId_;           ///< Page ID for the next page to be allocated.
    friend Page;
    public:
        /// Constructor
        PageManager(size_t maxBytes, size_t maxNode) : nextPageId_(1), maxNode_(maxNode) {  // Start with page ID 1.
            if (maxBytes < MINIMUM_FILE_BYTES) {
                maxBytes = MINIMUM_FILE_BYTES;
            }
            maxBytes_ = maxBytes;

            pages.push_back(Page(nextPageId_, maxBytes_, maxNode_));  // Initialize the first page.
        }

        ~PageManager() {
            for (auto& p : pages) {
                delete []p.page_;
                delete []p.bitSet_.data;
            }
        }

        /// Allocate a page
        Page* AllocatePage(long pageID, size_t size) {
            // If the requested pageID is larger than the current pages, we must add a new page.
            if (pageID > pages.size()) {
                pages.push_back(Page(pageID, maxBytes_, maxNode_));  // Create a new page
                nextPageId_ = pageID + 1;  // Update the nextPageId_ for future allocations
            }

            // Loop through the pages to find the first available page with free slots
            while (pageID <= pages.size()) {
                Page& targetPage = pages[pageID - 1];
                if (targetPage.findFreeSlot(size) != -1) { // Check if there's a free slot
                    return &targetPage;
                }
                // If no free slots, increment the pageID
                pageID++;
            }

            // If no available page found, create a new page and update nextPageId_
            pages.push_back(Page(++nextPageId_, maxBytes_, maxNode_));
            return &pages.back();
        }

        /// Assign a free slot in a page
        std::pair<long, int> Assign(long pageID, size_t size) {
            Page* page = AllocatePage(pageID, size);  // Ensure page exists or is created

            int slot = page->findFreeSlot(size);    // Find the first free slot
            page->allocateSlot(slot);           // Allocate that slot

            return {page->GetPageId(), slot};   // Return the page ID and slot number
        }

        /// Get a page by its ID
        Page* getPage(long pageID) {
            if (pageID <= pages.size()) {
                return &pages[pageID - 1];  // Return the page if it exists
            }
            return nullptr;  // Return nullptr if the page does not exist
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
