/**
 * @file cache.h
 * @brief Cache header file for the LevelDB implementation.
 *
 * This code is based on the cache implementation from the LevelDB project.
 * The original implementation can be found at:
 * https://github.com/google/leveldb
 *
 * Credit: Cache implementation by Google (LevelDB).
 *
 * @note This code is based on the original work in the `google/leveldb`
 * repository.
 *
 * @license Apache License, Version 2.0
 *
 * Copyright 2012 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TAGFILTERDB_CACHE_V2_H
#define TAGFILTERDB_CACHE_V2_H

#include "dataView.h"
#include "murmurHash.h"
#include <cassert>
#include <iostream>
#include <mutex>

namespace tagfilterdb {

struct CacheResponse {
    virtual ~CacheResponse() = default;

    virtual DataView GetKey() const = 0;

    virtual void *GetValue() const = 0;
};

class Cache {
  public:
    virtual ~Cache() = default;

    virtual CacheResponse *
    Insert(const DataView &key, void *value, size_t charge,
           void (*deleter)(const DataView &, void *value)) = 0;

    virtual CacheResponse *Get(const DataView &key) = 0;

    virtual bool Remove(const DataView &key) = 0;

    virtual void Prune() = 0;

    virtual CacheResponse *Release(CacheResponse *node) = 0;

    virtual void Print() const = 0;

    virtual size_t TotalUsage() const = 0;

    virtual size_t TotalCharge() const = 0;
};

class LRUCache final : public Cache {
  public:
    // Configuration for the LRU cache
    struct Config {
        size_t CACHE_TOTAL_CHANGE = 1000; ///<  total charge for the cache.
        size_t CACHE_EXPAND = 2;          ///<  expansion factor for the cache.
        size_t CACHE_RATIO = 0.8;         ///<  cache ratio for expansion.
    };

    // BucketNode class to store cache data
    class BucketNode {
      protected:
        BucketNode *next_;  ///< Pointer to the next node in the bucket.
        BucketNode *lNext_; ///< Pointer to the next node in the LRU list.
        BucketNode *lPrev_; ///< Pointer to the previous node in the LRU list.

        BucketNode() : next_(nullptr), lNext_(nullptr), lPrev_(nullptr) {}
        virtual ~BucketNode() = default;

        friend LRUCache;
    };

    // CachedBucketNode class to store cached data
    class CachedBucketNode final : public BucketNode, public CacheResponse {
      protected:
        char *key_;       ///< The key of the cache item.
        size_t key_size_; ///< The size of the key.
        uint32_t hash_;   ///< The hash of the cache item.
        void *value_;     ///< The value of the cache item.
        ssize_t size_;    ///< The size of the cache item.
        size_t charge_;   ///< The charge (size) of the cache item.
        size_t ref_ = 1;  ///< The reference count of the cache item.
        void (*deleter_)(const DataView &,
                         void *value); /// The deleter function
                                       /// for the cache item.

      public:
        CachedBucketNode(const DataView &key, uint32_t hash, void *value,
                         size_t charge,
                         void (*deleter)(const DataView &, void *value))
            : value_(value), charge_(charge), hash_(hash), deleter_(deleter) {
            key_size_ = key.size();
            key_ = new char[key_size_];
            memcpy(key_, key.data(), key_size_);
        }

        void *GetValue() const override {
            assert(this);
            return value_;
        }

        DataView GetKey() const override {
            assert(this);
            return DataView(key_, key_size_);
        }

        friend LRUCache;
    };

  public:
    // default constructor for LRUCache
    LRUCache() : config_(Config{}) { setup(); }
    /**
     * @brief Constructor that initializes the cache with a specified capacity
     * and total charge.
     * @param cap The capacity of the cache.
     * @param total_change The total charge available for the cache.
     */
    explicit LRUCache(Config config) : config_(config) { setup(); }

    /**
     * @brief Destructor that cleans up the allocated memory.
     */
    ~LRUCache() {
        for (size_t i = 0; i < capacity_; i++) {
            BucketNode *curr = data_[i].next_;
            while (curr != nullptr) {
                BucketNode *next = curr->next_;
                // if curr can cast to CachedBucketNode delete the key
                if (isCachedBucketNode(curr)) {
                    delete[] ((CachedBucketNode *)curr)->key_;
                }
                delete curr;
                curr = next;
            }
        }
        delete[] data_;
        delete outdated_head_;
        delete outdated_tail_;
        delete inUsed_head_;
        delete inUsed_tail_;
    }

    static LRUCache *Create(Config config) { return new LRUCache(config); }

    /**
     * @brief Inserts a new key-value pair into the cache.
     * @param key The key to insert.
     * @param value The value to insert.
     * @param charge The charge (size) of the item to insert.
     * @return A pointer to the inserted cache node.
     */
    CachedBucketNode *Insert(const DataView &key, void *value, size_t charge,
                             void (*deleter)(const DataView &,
                                             void *value)) override {
        // print value cast to string
        uint32_t hash = support::MurmurHash::Hash(key.data(), key.size(), 0);
        return Insert(key, hash, value, charge, deleter);
    }

    /**
     * @brief Inserts a new key-value pair into the cache.
     * @param key The key to insert.
     * @param hash The hash of the key.
     * @param value The value to insert.
     * @param charge The charge (size) of the item to insert.
     * @return A pointer to the inserted cache node.
     */
    CachedBucketNode *Insert(const DataView &key, uint32_t hash, void *value,
                             size_t charge,
                             void (*deleter)(const DataView &, void *value)) {
        if (charge > config_.CACHE_TOTAL_CHANGE) {
            return nullptr;
        }
        assert(charge > 0);

        CacheMux m(&mux_);

        if (isExpand()) {
            expand(capacity_ * config_.CACHE_EXPAND);
        }

        CachedBucketNode *newNode =
            new CachedBucketNode(key, hash, value, charge, deleter);

        size_t index = hash % capacity_;
        BucketNode *prev = &data_[index];

        while (prev != nullptr && prev->next_ != nullptr) {
            if (((CachedBucketNode *)(prev->next_))->GetKey() == key) {
                break;
            }
            prev = prev->next_;
        }
        if (prev->next_ == nullptr) {
            prev->next_ = newNode;
            size_++;
        } else {
            // The key already existed
            newNode->next_ = prev->next_->next_;

            removeList(prev->next_);
            total_usage_ -= ((CachedBucketNode *)(prev->next_))->charge_;
            delete prev->next_;

            prev->next_ = newNode;
        }

        // Remove in OutDated List if excess a total charge
        while (total_usage_ + charge > config_.CACHE_TOTAL_CHANGE &&
               outdated_tail_->lPrev_ != outdated_head_) {
            bool s = removeNode(
                ((CachedBucketNode *)(outdated_head_->lNext_))->GetKey());
            assert(s);
        }

        appendToList(newNode, inUsed_tail_);
        total_usage_ += charge;

        assert(total_usage_ > 0);
        assert(total_usage_ <= config_.CACHE_TOTAL_CHANGE);
        ref(newNode);
        return newNode;
    }

    /**
     * @brief Removes a key-value pair from the cache.
     * @param key The key to remove.
     * @return True if the item was successfully removed, false otherwise.
     */
    bool Remove(const DataView &key) override {
        CacheMux m(&mux_);
        return removeNode(key);
    }

    /**
     * @brief Retrieves a value from the cache using a key.
     * @param key The key to search for.
     * @return A pointer to the cache node if found, nullptr otherwise.
     */
    CacheResponse *Get(const DataView &key) override {
        CacheMux m(&mux_);

        uint32_t hash = support::MurmurHash::Hash(key.data(), key.size(), 0);
        size_t index = hash % capacity_;
        CachedBucketNode *curr = (CachedBucketNode *)data_[index].next_;
        while (curr != nullptr) {
            if (curr->GetKey() == key) {
                ref(curr);
                return (CacheResponse *)curr;
            }
            curr = (CachedBucketNode *)curr->next_;
        }
        return nullptr;
    }

    /**
     * @brief Prunes the cache by removing all outdated nodes.
     */
    void Prune() override {
        BucketNode *curr = outdated_head_->lNext_;
        while (curr != outdated_tail_) {
            BucketNode *next = curr->lNext_;
            assert((((CachedBucketNode *)curr)->ref_ == 1));
            bool s = removeNode(((CachedBucketNode *)curr)->GetKey());
            assert(s);
            curr = next;
        }
    }

    /**
     * @brief Releases a cache node, decreasing its reference count.
     * @param node The node to release.
     */
    CacheResponse *Release(CacheResponse *node) override {
        if (node != nullptr) {
            unref((CachedBucketNode *)node);
        }
        return node;
    }

    /**
     * @brief Prints the current state of the cache.
     */
    void Print() const override {
        for (size_t i = 0; i < capacity_; i++) {
            std::cout << i << " ";
            CachedBucketNode *curr = (CachedBucketNode *)data_[i].next_;
            while (curr != nullptr) {
                std::cout << "(" << curr->key_ << ", " << curr->value_ << ", "
                          << curr->charge_ << ", " << curr->ref_ << ") ";
                curr = (CachedBucketNode *)curr->next_;
            }
            std::cout << std::endl;
        }
    }

    Config &EditConfig() { return config_; }

    /**
     * @brief Returns the total charge of the cache.
     * @return The total charge.
     */
    size_t TotalCharge() const override { return config_.CACHE_TOTAL_CHANGE; }

    /**
     * @brief Returns the total usage of the cache.
     * @return The total usage.
     */
    size_t TotalUsage() const override { return total_usage_; }

    /**
     * @brief Prints the nodes in the "outdated" list.
     */
    void PrintOutDated() const {
        BucketNode *curr = outdated_head_->lNext_;
        std::cout << "OutDated: ";
        while (curr != outdated_tail_) {
            CachedBucketNode *node = (CachedBucketNode *)curr;
            std::cout << "(" << node->key_ << ", " << node->value_ << ") ";
            curr = curr->lNext_;
        }
        std::cout << std::endl;
    }

    /**
     * @brief Prints the nodes in the "in-use" list.
     */
    void PrintInUsed() const {
        BucketNode *curr = inUsed_head_->lNext_;
        std::cout << "InUsed: ";
        while (curr != inUsed_tail_) {
            CachedBucketNode *node = (CachedBucketNode *)curr;
            std::cout << "(" << node->key_ << ", " << node->value_ << ") ";
            curr = curr->lNext_;
        }
        std::cout << std::endl;
    }

    /**
     * @brief Prints detailed information about the cache.
     */
    void Detail() const {
        std::cout << "Detail:" << std::endl;
        std::cout << "- Capacity: " << capacity_ << std::endl;
        std::cout << "- Size: " << size_ << std::endl;
        std::cout << "- Total Charge: " << config_.CACHE_TOTAL_CHANGE
                  << std::endl;
        std::cout << "- Total Usage: " << total_usage_ << std::endl;
    }

    /**
     * @brief Retrieves the key of a cache node.
     * @param node The cache node.
     * @return The key of the cache item.
     */
    static std::string GetKey(CachedBucketNode *node) {
        if (node == nullptr) {
            return "";
        }
        return node->key_;
    }

  private:
    class CacheMux {
      protected:
        std::mutex *mux_;
        CacheMux(std::mutex *mux) {
            this->mux_ = mux;
            mux_->lock();
        }
        ~CacheMux() { mux_->unlock(); }

        friend LRUCache;
    };

    void setup() {
        capacity_ = 2;
        assert(capacity_ > 0);
        assert(config_.CACHE_TOTAL_CHANGE > 0);

        data_ = new BucketNode[capacity_];

        size_ = 0;
        total_usage_ = 0;

        outdated_head_ = new BucketNode;
        outdated_tail_ = new BucketNode;
        outdated_head_->lNext_ = outdated_tail_;
        outdated_head_->lPrev_ = outdated_tail_;
        outdated_tail_->lNext_ = outdated_head_;
        outdated_tail_->lPrev_ = outdated_head_;

        inUsed_head_ = new BucketNode;
        inUsed_tail_ = new BucketNode;
        inUsed_head_->lNext_ = inUsed_tail_;
        inUsed_head_->lPrev_ = inUsed_tail_;
        inUsed_tail_->lNext_ = inUsed_head_;
        inUsed_tail_->lPrev_ = inUsed_head_;
    }

    void ref(CachedBucketNode *refNode) {
        assert(refNode->ref_ >= 1);
        refNode->ref_++;
        if (refNode->ref_ == 2) {
            removeList(refNode);
            appendToList(refNode, inUsed_tail_);
        }
    }

    void unref(CachedBucketNode *refNode) {
        assert(refNode->ref_ >= 1);
        refNode->ref_--;
        if (refNode->ref_ == 1) {
            removeList(refNode);
            appendToList(refNode, outdated_tail_);
        } else if (refNode->ref_ == 0) {
            removeNode(refNode->GetKey());
        }
    }

    bool isExpand() { return capacity_ * config_.CACHE_RATIO < size_; }

    bool isCachedBucketNode(BucketNode *node) {
        return dynamic_cast<CachedBucketNode *>(node) != nullptr;
    }

    void expand(size_t newCap) {
        BucketNode *newLoc = new BucketNode[newCap];
        // rehash
        for (size_t i = 0; i < capacity_; i++) {
            CachedBucketNode *curr = (CachedBucketNode *)data_[i].next_;
            while (curr != nullptr) {
                CachedBucketNode *next = (CachedBucketNode *)curr->next_;
                curr->next_ = nullptr;
                size_t index =
                    support::MurmurHash::Hash(curr->GetKey().data(),
                                              curr->GetKey().size(), 0) %
                    newCap;
                if (newLoc[index].next_ == nullptr) {
                    newLoc[index].next_ = curr;
                } else {
                    BucketNode *b = newLoc[index].next_;
                    while (b->next_ != nullptr) {
                        b = b->next_;
                    }
                    b->next_ = curr;
                }
                curr = next;
            }
        }
        delete[] data_;
        data_ = newLoc;
        capacity_ = newCap;
    }

    bool removeNode(const DataView &key) {
        size_t index =
            support::MurmurHash::Hash(key.data(), key.size(), 0) % capacity_;
        CachedBucketNode *curr = (CachedBucketNode *)data_[index].next_;
        BucketNode *prev = &data_[index];

        while (curr != nullptr && curr->GetKey() != key) {
            prev = curr;
            curr = (CachedBucketNode *)curr->next_;
        }
        if (curr == nullptr) {
            return false;
        }

        if (curr->deleter_ != nullptr) {
            (*curr->deleter_)(curr->GetKey(), curr->GetValue());
        }

        delete curr->key_;
        total_usage_ -= curr->charge_;
        size_--;
        prev->next_ = curr->next_;
        removeList(curr);
        delete curr;
        return true;
    }

    void appendToList(BucketNode *node, BucketNode *tail) {
        assert(node != nullptr);
        BucketNode *prev = tail->lPrev_;
        node->lNext_ = tail;
        node->lPrev_ = prev;
        prev->lNext_ = node;
        tail->lPrev_ = node;
    }

    void removeList(BucketNode *node) {
        assert(node != nullptr);
        if (node->lNext_ == nullptr && node->lPrev_ == nullptr) {
            return;
        }
        BucketNode *next = node->lNext_;
        BucketNode *prev = node->lPrev_;
        prev->lNext_ = next;
        next->lPrev_ = prev;

        node->lNext_ = nullptr;
        node->lPrev_ = nullptr;
    }

  private:
    BucketNode *data_;   ///< Array of bucket nodes to store the cache data.
    size_t size_;        ///< The current size of the cache.
    size_t capacity_;    ///< The capacity of the cache.
    size_t total_usage_; ///< The total usage of cache space.

    BucketNode *inUsed_head_; ///< Head of the LRU "in-use" list.
    BucketNode *inUsed_tail_; ///< Tail of the LRU "in-use" list.

    BucketNode *outdated_head_; ///< Head of the LRU "outdated" list.
    BucketNode *outdated_tail_; ///< Tail of the LRU "outdated" list.

    std::mutex mux_; ///< Mutex used to synchronize cache access.

    Config config_; ///< Configuration for the LRU cache.
};

class ShareLRUCache final : public Cache {
  public:
    struct Config {
        size_t SHARECACHE_BIT = 4; ///< Default bit of LRU caches.
        size_t SHARECACHE_N = 1 << SHARECACHE_BIT; ///< Default number of
                                                   ///< LRU caches.
        size_t SHARECACHE_TOTAL_CHARGE = 4000; ///< Default total charge to be
                                               ///< divided among caches.
    };

    /**
     * @brief Constructs a shared LRU cache with a specified number of
     caches.
     * @param charge The total charge to divide among caches.
     */
    ShareLRUCache(Config config) {
        assert(config.SHARECACHE_TOTAL_CHARGE > 0);
        config_ = config;
        // new with config
        LRUCache::Config lru_config = LRUCache::Config{
            .CACHE_TOTAL_CHANGE =
                (config_.SHARECACHE_TOTAL_CHARGE + config_.SHARECACHE_N - 1) /
                config_.SHARECACHE_N,
        };
        // new with config
        m_caches = new LRUCache[config_.SHARECACHE_N];
        // m_caches = new LRUCache(config_)[config_.SHARECACHE_N];

        // m_caches = new LRUCache[config_.SHARECACHE_N];
        for (size_t i = 0; i < config_.SHARECACHE_N; i++) {
            m_caches[i].EditConfig().CACHE_TOTAL_CHANGE =
                (config_.SHARECACHE_TOTAL_CHARGE + config_.SHARECACHE_N - 1) /
                config_.SHARECACHE_N;
            m_caches[i].EditConfig().CACHE_EXPAND = 2;
            m_caches[i].EditConfig().CACHE_RATIO = 0.8;
        }
    }

    /**
     * @brief Destructor that cleans up the allocated memory.
     */
    ~ShareLRUCache() { delete[] m_caches; }

    static ShareLRUCache *Create(Config config) {
        return new ShareLRUCache(config);
    }

    /**
     * @brief Inserts a new key-value pair into the shared cache.
     * @param key The key to insert.
     * @param value The value to insert.
     * @param charge The charge (size) of the item to insert.
     * @return A pointer to the inserted cache node.
     */
    CacheResponse *Insert(const DataView &key, void *value, size_t charge,
                          void (*deleter)(const DataView &,
                                          void *value)) override {
        uint32_t hash = support::MurmurHash::Hash(key.data(), key.size(), 0);
        return (CacheResponse *)m_caches[Shard(hash)].Insert(key, hash, value,
                                                             charge, deleter);
    }

    /**
     * @brief Removes a key-value pair from the shared cache.
     * @param key The key to remove.
     */
    bool Remove(const DataView &key) override {
        uint32_t hash = support::MurmurHash::Hash(key.data(), key.size(), 0);
        return m_caches[Shard(hash)].Remove(key);
    }

    /**
     * @brief Retrieves a value from the shared cache using a key.
     * @param key The key to search for.
     * @return A pointer to the cache node if found, nullptr otherwise.
     */
    CacheResponse *Get(const DataView &key) override {
        uint32_t hash = support::MurmurHash::Hash(key.data(), key.size(), 0);
        return m_caches[Shard(hash)].Get(key);
    }

    /**
     * @brief Calculates the total cache usage across all individual caches.
     * @return The total usage of the shared cache.
     */
    size_t TotalUsage() const override {
        size_t t = 0;
        for (size_t i = 0; i < config_.SHARECACHE_N; i++) {
            t += m_caches[i].TotalUsage();
        }
        return t;
    }

    /**
     * @brief Returns the total charge of the cache.
     * @return The total charge.
     */
    size_t TotalCharge() const override {
        return config_.SHARECACHE_TOTAL_CHARGE;
    }

    /**
     * @brief Prints the state of each individual LRU cache in the shared
     cache.
     */
    void Print() const override {
        for (size_t i = 0; i < config_.SHARECACHE_N; i++) {
            std::cout << "Cache: " << i + 1 << " =====" << std::endl;
            m_caches[i].Print();
            m_caches[i].Detail();
            m_caches[i].PrintInUsed();
            m_caches[i].PrintOutDated();
            std::cout << std::endl;
        }
    }

    /**
     * @brief Prints detailed information about the shared cache and its
     individual caches.
     */
    void Detail() {
        std::cout << "Total Charge: " << config_.SHARECACHE_TOTAL_CHARGE
                  << std::endl;
        std::cout << "Total Usage: " << TotalUsage() << std::endl;
        // for (size_t i = 0 ; i < config_.SHARECACHE_N; i++) {
        //     std::cout <<"Cache: " << i + 1 << " =====" << std::endl;
        //     m_caches[i].Detail();
        // }
    }

    /**
     * @brief Retrieves the LRU cache at a specified index.
     * @param index The index of the LRU cache to retrieve.
     * @return A pointer to the specified LRU cache.
     */
    LRUCache *GetLRU(size_t index) { return &m_caches[index]; }

    /**
     * @brief Prunes all individual caches by removing outdated nodes.
     */
    void Prune() override {
        for (size_t i = 0; i < config_.SHARECACHE_N; i++) {
            m_caches[i].Prune();
        }
    }

    /**
     * @brief Releases a cache node, decreasing its reference count.
     * @param bnode The cache node to release.
     */
    CacheResponse *Release(CacheResponse *node) {
        if (node == nullptr) {
            return nullptr;
        }
        uint32_t hash = support::MurmurHash::Hash(node->GetKey().data(),
                                                  node->GetKey().size(), 0);
        m_caches[Shard(hash)].Release(node);
        return node;
    }

    /**
     * @brief Retrieves the value stored in a cache node.
     * @param bnode The cache node.
     * @return The value of the cache item.
     */
    static void *GetValue(CacheResponse *node) { return node->GetValue(); }

    /**
     * @brief Retrieves the key of a cache node.
     * @param bnode The cache node.
     * @return The key of the cache item.
     */
    static DataView GetKey(CacheResponse *node) { return node->GetKey(); }

  private:
    uint32_t Shard(uint32_t hash) {
        return hash >> (32 - config_.SHARECACHE_BIT);
    }

    LRUCache *m_caches; ///< Array of LRUCache instances.

    Config config_; ///< Configuration for the shared LRU cache.
};

} // namespace tagfilterdb

#endif