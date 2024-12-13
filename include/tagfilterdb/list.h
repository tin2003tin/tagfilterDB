#ifndef TAGFILTERDB_LINK_LIST_H
#define TAGFILTERDB_LINK_LIST_H

#include <atomic>
#include <mutex>
#include <stdexcept>
#include <iterator>

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
        std::atomic<Node*> head_;
        std::atomic<Node*> tail_;
        std::atomic<size_t> size_;
        mutable std::mutex mtx_;
        Arena* arena_;

    public:
        // Constructor to initialize an empty list
        List(Arena* arena) : head_(nullptr), tail_(nullptr), size_(0), arena_(arena) {}

        Value* Add(Value data) {
            std::lock_guard<std::mutex> lock(mtx_);

            char* const node_memory = arena_->AllocateAligned(sizeof(Node));
            Node* newNode = new (node_memory) Node(data);

            Node* oldTail = tail_.load(std::memory_order_relaxed);

            if (oldTail) {
                oldTail->next_ = newNode;
                newNode->prev_ = oldTail;
                tail_.store(newNode, std::memory_order_release);
            } else {
                head_.store(newNode, std::memory_order_release);
                tail_.store(newNode, std::memory_order_release);
            }

            size_.fetch_add(1, std::memory_order_relaxed);
            return &newNode->data_;
        }

        size_t GetSize() const {
            return size_.load(std::memory_order_acquire);
        }

        // Iterator class
        class Iterator {
            Node* current_;
            Node* prev_;  // Tracks the previous node for deletion
            List* list_;

        public:
            explicit Iterator(Node* node, List* list) 
                : current_(node), prev_(nullptr), list_(list) {}

            Value& operator*() {
                return current_->data_;
            }

            Value* operator->() {
                return &current_->data_;
            }

            Iterator& operator++() {
                current_ = current_->next_;
                return *this;
            }

            bool operator==(const Iterator& other) const {
                return current_ == other.current_;
            }

            bool operator!=(const Iterator& other) const {
                return current_ != other.current_;
            }
        };

        Iterator begin() {
            return Iterator(head_.load(std::memory_order_acquire), this);
        }

        Iterator end() {
            return Iterator(nullptr, this);
        }

    private:
   
    };

}

#endif
