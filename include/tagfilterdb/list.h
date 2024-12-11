#ifndef TAGFILTERDB_LINK_LIST_H
#define TAGFILTERDB_LINK_LIST_H

#include <atomic>
#include <mutex>
#include <stdexcept>

namespace tagfilterdb {

    template <class Value>
    class List {
    public:
        struct Node {
            Value data_;              // The data (can be any type)
            Node* next_;              // Pointer to the next node
            Node* prev_;              // Pointer to the previous node

            // Constructor to initialize a node with the given value
            Node(Value data) : data_(data), next_(nullptr), prev_(nullptr) {}
        };

    private:
        std::atomic<Node*> head;   // Atomic pointer to the head of the list (first block)
        std::atomic<Node*> tail;   // Atomic pointer to the tail of the list (last block)
        std::atomic<size_t> size;  // Atomic size of the list
        mutable std::mutex mtx;    // Mutex to protect against concurrent modifications

    public:
        // Constructor to initialize an empty list
        List() : head(nullptr), tail(nullptr), size(0) {}

        // Add a block to the list (takes a Value type)
        void Add(Value data) {
            std::lock_guard<std::mutex> lock(mtx);  // Lock the list during modification

            Node* newNode = new Node(data);
            Node* oldTail = tail.load(std::memory_order_relaxed);

            if (oldTail) {
                oldTail->next_ = newNode;  // Link the current tail to the new node
                newNode->prev_ = oldTail;  // Set the previous pointer of the new node
                tail.store(newNode, std::memory_order_release); // Update the tail atomically
            } else {
                head.store(newNode, std::memory_order_release); // If the list is empty, both head and tail point to the new node
                tail.store(newNode, std::memory_order_release);
            }

            size.fetch_add(1, std::memory_order_relaxed);  // Atomically increment the size
        }

        // Remove a block from the list (returns the block's data)
        Value Remove() {
            std::lock_guard<std::mutex> lock(mtx);  // Lock the list during modification

            if (head.load(std::memory_order_acquire) == nullptr) {
                throw std::out_of_range("List is empty");
            }

            Node* nodeToRemove = head.load(std::memory_order_acquire);  // Get the node at the head
            Value data = nodeToRemove->data_;                            // Get the data
            Node* newHead = nodeToRemove->next_;

            if (newHead) {
                newHead->prev_ = nullptr;  // Update the new head's previous pointer to null
                head.store(newHead, std::memory_order_release);  // Update the head atomically
            } else {
                tail.store(nullptr, std::memory_order_release);  // If the list is empty after removal, reset tail
            }

            delete nodeToRemove;  // Free the memory of the removed node
            size.fetch_sub(1, std::memory_order_relaxed);  // Atomically decrement the size

            return data;  // Return the freed block's data
        }

        // Check if the list is empty
        bool IsEmpty() const {
            return head.load(std::memory_order_acquire) == nullptr;
        }

        // Get the current size of the list
        size_t GetSize() const {
            return size.load(std::memory_order_acquire);
        }

        // Destructor to clean up the list nodes
        ~List() {
            while (!IsEmpty()) {
                Remove();  // Clean up the list by removing each node
            }
        }
    };

}

#endif
