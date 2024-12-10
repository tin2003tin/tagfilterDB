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
 * @note This code is based on the original work in the `google/leveldb` repository.
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


#ifndef TAGFILTERDB_CACHE_H
#define TAGFILTERDB_CACHE_H

#include <iostream>
#include <cassert>
#include <mutex>
#include "murmurHash.h"

namespace tagfilterdb {

/**
 * @struct BaseBucketNode
 * @brief A base structure for nodes in cache buckets.
 */
struct BaseNode {};

/**
 * @class LRUConfig
 * @brief A configuration class to hold cache-related constants.
 */
class LRUConfig {
public:
    static const double DEFAULT_CACHE_RATIO; ///< The default cache ratio for expansion.
    static const size_t DEFAULT_CACHE_CAP; ///< The default cache capacity.
    static const size_t DEFAULT_CAHCE_EXPAND; ///< The default expansion factor for the cache.
    static const size_t DEFAULT_CACHE_CHARGE_PER; ///< The default charge per cache item.
    static const size_t DEFAULT_CACHE_TOTAL_CHANGE; ///< The default total cache charge.
};

const double LRUConfig::DEFAULT_CACHE_RATIO = 0.8;
const size_t LRUConfig::DEFAULT_CACHE_CAP = 2;
const size_t LRUConfig::DEFAULT_CAHCE_EXPAND = 2;
const size_t LRUConfig::DEFAULT_CACHE_CHARGE_PER = 8;
const size_t LRUConfig::DEFAULT_CACHE_TOTAL_CHANGE = 1000;

/**
 * @class LRUCache
 * @brief A Least Recently Used (LRU) Cache implementation.
 * 
 * This class manages a cache using the LRU eviction policy. It stores key-value pairs 
 * and ensures that the least recently used items are evicted when the cache exceeds its 
 * total charge limit.
 *
 * @tparam Value Type of the value in the cache.
 */
template <typename Value>
class LRUCache {
    public :

    /**
     * @class CacheMux
     * @brief A class to handle locking mechanisms for cache access.
     */
    class CacheMux {
        protected:
        std::mutex* mux_;
        CacheMux(std::mutex* mux) {
            this->mux_ = mux;
            mux_->lock();
        }
        ~CacheMux() {
            mux_->unlock();
        }

        friend LRUCache;
    };

     /**
     * @class BucketNode
     * @brief A class representing a node in the bucket list.
     */
    class BucketNode {
        protected:
        BucketNode* next_; ///< Pointer to the next node in the bucket.
        BucketNode* lNext_; ///< Pointer to the next node in the LRU list.
        BucketNode* lPrev_; ///< Pointer to the previous node in the LRU list.

        BucketNode() : next_(nullptr), lNext_(nullptr), lPrev_(nullptr) {}
        virtual ~BucketNode() = default;

        friend LRUCache;
    };

     /**
     * @class Referenceable
     * @brief A class that manages reference counting for cache nodes.
     */
    class Referenceable
    {
        protected:
        size_t ref_; ///< Reference count for the node.

        Referenceable() : ref_(1) {}

        friend LRUCache;
    };
    
    /**
     * @class BucketValueNode
     * @brief A class representing a value node in the cache.
     */
    class BucketValueNode final : public BucketNode, public Referenceable, public BaseNode {
        protected:
        std::string key_; ///< The key of the cache item.
        uint32_t hash_;
        Value value_; ///< The value of the cache item.
        size_t charge_; ///< The charge (size) of the cache item.

        BucketValueNode(std::string key, Value value, size_t charge,uint32_t hash) : 
        key_(key), value_(value), charge_(charge), hash_(hash) {
    }       
        public:
        /**
         * @brief Returns the value stored in the cache node.
         * @return The value of the cache item.
         */
        Value& getValue() {
            assert(this);
            return value_; 
        }

         /**
         * @brief Returns the key of the cache node.
         * @return The key of the cache item.
         */
        std::string getKey() {
            assert(this);
            return key_;
        }

        friend LRUCache;
    };
    private: 
    BucketNode* data_; ///< Array of bucket nodes to store the cache data.
    size_t cap_; ///< The capacity of the cache.
    size_t size_; ///< The current size of the cache.
    size_t total_charge_; ///< The total charge available for the cache.
    size_t total_usage_; ///< The total usage of cache space.

    BucketNode* inUsed_head_; ///< Head of the LRU "in-use" list.
    BucketNode* inUsed_tail_; ///< Tail of the LRU "in-use" list.

    BucketNode* outdated_head_; ///< Head of the LRU "outdated" list.
    BucketNode* outdated_tail_; ///< Tail of the LRU "outdated" list.
    
    std::mutex mux_; ///< Mutex used to synchronize cache access.

    public: 
    
     /**
     * @brief Default constructor that initializes the cache with default configuration.
     */
    LRUCache() {
        setup(LRUConfig::DEFAULT_CACHE_CAP, LRUConfig::DEFAULT_CACHE_TOTAL_CHANGE);
    }

    /**
     * @brief Constructor that initializes the cache with a specified capacity and total charge.
     * @param cap The capacity of the cache.
     * @param total_change The total charge available for the cache.
     */
    LRUCache(size_t cap, size_t total_change = LRUConfig::DEFAULT_CACHE_TOTAL_CHANGE) {
        setup(cap,total_change);
    }

    /**
     * @brief Destructor that cleans up the allocated memory.
     */
    ~LRUCache() { 
        for (size_t i = 0 ; i < cap_; i++) {
            BucketNode* curr = data_[i].next_;
            while (curr != nullptr) {
                BucketNode* next = curr->next_;
                delete curr;
                curr = next;
            }
        }
        delete []data_;
        delete outdated_head_;
        delete outdated_tail_;
        delete inUsed_head_;
        delete inUsed_tail_;
    } 

    /**
     * @brief Sets the charge for the cache.
     * @param charge The new total charge for the cache.
     */
    void SetCharge(size_t charge) {
        total_charge_ = charge;
    }

    /** 
     * @brief Inserts a new key-value pair into the cache.
     * @param key The key to insert.
     * @param value The value to insert.
     * @param charge The charge (size) of the item to insert.
     * @return A pointer to the inserted cache node.
     */
     BucketValueNode* Insert(std::string key, Value value, size_t charge = LRUConfig::DEFAULT_CACHE_CHARGE_PER) {
        uint32_t hash = support::MurmurHash::Hash(key.data(),key.size(),0);
        return Insert(key,value,hash,charge);
     }

    /**
     * @brief Inserts a new key-value pair into the cache.
     * @param key The key to insert.
     * @param value The value to insert.
     * @param hash The hash to insert.
     * @param charge The charge (size) of the item to insert.
     * @return A pointer to the inserted cache node.
     */
    BucketValueNode* Insert(std::string key, Value value,uint32_t hash, size_t charge = LRUConfig::DEFAULT_CACHE_CHARGE_PER) {
        if (charge > total_charge_) {
            return nullptr;
        }
        assert(charge > 0);

        CacheMux m(&mux_);

        if (isExpand()) {
            expand(cap_ * LRUConfig::DEFAULT_CAHCE_EXPAND);
        }

        BucketValueNode* newNode =  new BucketValueNode(key,value,charge,hash);

        size_t index = hash % cap_;
        BucketNode* prev = &data_[index];

        while (prev != nullptr && prev->next_ != nullptr)
        {
            if (((BucketValueNode* )(prev->next_))->key_ == key) {
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
            total_usage_ -= ((BucketValueNode* )(prev->next_))->charge_;
            delete prev->next_;

            prev->next_ = newNode;
        }

        // Remove in OutDated List if excess a total charge
        while (total_usage_ + charge > total_charge_ 
               && outdated_tail_->lPrev_ != outdated_head_) {
            bool s = removeNode(((BucketValueNode* )(outdated_head_->lNext_))->key_);
            assert(s);
        }

        appendToList(newNode, inUsed_tail_);
        total_usage_ += charge;

        assert(total_usage_ > 0);
        assert(total_usage_ <= total_charge_);
        ref(newNode);
        return newNode;
    }

    /**
     * @brief Removes a key-value pair from the cache.
     * @param key The key to remove.
     * @return True if the item was successfully removed, false otherwise.
     */
    bool Remove(std::string key) {
       CacheMux m(&mux_);
       return removeNode(key);
    }

    /**
     * @brief Retrieves a value from the cache using a key.
     * @param key The key to search for.
     * @return A pointer to the cache node if found, nullptr otherwise.
     */
    BucketValueNode* Get(std::string key) {
        CacheMux m(&mux_);

        uint32_t hash = support::MurmurHash::Hash(key.data(),key.size(),0);
        size_t index = hash % cap_;
        BucketValueNode* curr = (BucketValueNode*)  data_[index].next_;
        while (curr != nullptr) {
            if (curr->key_ == key) {
                ref(curr);
                return curr;
            }
            curr = (BucketValueNode*) curr->next_;
        }
        return nullptr;
    }

    /**
     * @brief Prunes the cache by removing all outdated nodes.
     */
    void Prune() {
        BucketNode* curr = outdated_head_->lNext_;
        while (curr != outdated_tail_) {
            BucketNode* next = curr->lNext_;
            assert((((BucketValueNode *) curr)->ref_ == 1));
            bool s = removeNode(((BucketValueNode *)curr)->key_);
            assert(s);
            curr = next;
        }
    }

     /**
     * @brief Releases a cache node, decreasing its reference count.
     * @param node The node to release.
     */
    void Release(BucketValueNode* node) {
        if (node != nullptr) {
            unref(node);
        }
    }

    /**
     * @brief Prints the current state of the cache.
     */
    void Print() const {
        for (size_t i = 0; i < cap_; i++) {
            std::cout << i << " ";
            BucketValueNode* curr = (BucketValueNode*)  data_[i].next_;
            while (curr != nullptr)
            {
                std::cout << "("<< curr->key_ << ", " << curr->value_ << ", " << curr->charge_<< ", " << curr->ref_ <<  ") ";
                curr =  (BucketValueNode*) curr->next_;
            }
            std::cout << std::endl;
        }
    }

     /**
     * @brief Returns the total charge of the cache.
     * @return The total charge.
     */
    size_t TotalCharge() const {
        return total_charge_;
    }

     /**
     * @brief Returns the total usage of the cache.
     * @return The total usage.
     */
    size_t TotalUsage() const {
        return total_usage_;
    }

     /**
     * @brief Prints the nodes in the "outdated" list.
     */
    void PrintOutDated() const {
        BucketNode* curr = outdated_head_->lNext_;
        std::cout << "OutDated: ";
        while (curr != outdated_tail_)
        {
            BucketValueNode* node = ( BucketValueNode* ) curr;
            std::cout << "(" << node->key_ << ", "  << node->value_ << ") ";
            curr = curr->lNext_;
        }
        std::cout << std::endl;
    }

    /**
     * @brief Prints the nodes in the "in-use" list.
     */
     void PrintInUsed() const {
        BucketNode* curr = inUsed_head_->lNext_;
        std::cout << "InUsed: ";
        while (curr != inUsed_tail_)
        {
            BucketValueNode* node = ( BucketValueNode* ) curr;
            std::cout << "(" << node->key_ << ", "  << node->value_ << ") ";
            curr = curr->lNext_;
        }
        std::cout << std::endl;
    }

     /**
     * @brief Prints detailed information about the cache.
     */
    void Detail() const {
        std::cout << "Detail:" << std::endl;
        std::cout << "- Capacity: " << cap_ << std::endl;
        std::cout << "- Size: " << size_ << std::endl;
        std::cout << "- Total Charge: " << total_charge_ << std::endl;
        std::cout << "- Total Usage: " << total_usage_ << std::endl;
    }

     /**
     * @brief Retrieves the value stored in a cache node.
     * @param node The cache node.
     * @return The value of the cache item.
     */
    static Value GetValue(BaseNode* node) {
        if (node == nullptr) {
            return Value();
        }
        return ((BucketValueNode *) node)->value_;
    }

    /**
     * @brief Retrieves the key of a cache node.
     * @param node The cache node.
     * @return The key of the cache item.
     */
    static std::string GetKey(BucketValueNode* node) {
        if (node == nullptr) {
            return "";
        }
        return node->key_;
    }


    private:
        void setup(size_t cap, size_t total_charge) {
            assert(cap > 0);
            assert(total_charge > 0);

            data_ = new BucketNode[cap];
            cap_ = cap;
            size_ = 0;
            total_charge_ = total_charge;
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

        void ref(BucketValueNode *refNode) {
            assert(refNode->ref_ >= 1);
            refNode->ref_++;
            if (refNode->ref_ == 2) {
                 removeList(refNode);
                appendToList(refNode, inUsed_tail_);
            }
        }

        void unref(BucketValueNode *refNode) {
            assert(refNode->ref_ >= 1);
            refNode->ref_--;
            if (refNode->ref_ == 1) {
                removeList(refNode);
                appendToList(refNode, outdated_tail_);
            } else if (refNode->ref_ == 0) {
                removeNode(refNode->key_);
            }
        }

        bool isExpand() {
            return cap_ * LRUConfig::DEFAULT_CACHE_RATIO <  size_;
        }

        bool isBucketValueNode(BucketNode* node) {
            return dynamic_cast<BucketValueNode*>(node) != nullptr;
        }

        void expand(size_t newCap) {
            BucketNode* newLoc = new BucketNode[newCap];    
            // rehash
            for (size_t i = 0 ; i < cap_; i++) {
                BucketValueNode* curr = (BucketValueNode*) data_[i].next_;
                while (curr != nullptr) {
                    BucketValueNode* next = (BucketValueNode*)  curr->next_;
                    curr->next_ = nullptr;
                    size_t index = support::MurmurHash::Hash(curr->key_.data(),curr->key_.size(),0) % newCap;  
                    if (newLoc[index].next_ == nullptr) {
                        newLoc[index].next_ = curr;
                    } else {
                        BucketNode* b = newLoc[index].next_;
                        while (b->next_ != nullptr)
                        {
                            b= b->next_;
                        }
                        b->next_ = curr;
                    }
                    curr = next;
                }
            }
            delete[] data_;
            data_ = newLoc;
            cap_ = newCap;
        }

        bool removeNode(std::string key) {
            size_t index = support::MurmurHash::Hash(key.data(),key.size(),0) % cap_;  
            BucketValueNode* curr = (BucketValueNode*)  data_[index].next_;
            BucketNode* prev = &data_[index];
            while (curr != nullptr && curr->key_ != key) {
                    prev = curr;
                    curr = (BucketValueNode*)  curr->next_;
            } 
            if (curr == nullptr) {
                return false;
            }
            total_usage_ -= curr->charge_; 
            size_--;
            prev->next_ = curr->next_;
            removeList(curr);
            delete curr;
            return true;
        }

        void appendToList(BucketNode* node, BucketNode* tail) {
            assert(node != nullptr);
            BucketNode* prev = tail->lPrev_;
            node->lNext_ = tail;
            node->lPrev_ = prev;
            prev->lNext_ = node;
            tail->lPrev_ = node;
        }

        void removeList(BucketNode* node) {
            assert(node != nullptr);
            if (node->lNext_ == nullptr && node->lPrev_ == nullptr) {
                return;
            }
            BucketNode* next = node->lNext_;                
            BucketNode* prev = node->lPrev_;
            prev->lNext_ = next;
            next->lPrev_ = prev;

            node->lNext_ = nullptr;
            node->lPrev_ = nullptr;
        }
};

/**
 * @class ShareLRUConfig
 * @brief A configuration class to hold constants related to the shared LRU cache.
 */
class ShareLRUConfig {
public:
    static const size_t DEFAULT_SHARECACHE_BIT; ///< Default bit of LRU caches.
    static const size_t DEFAULT_SHARECACHE_N; ///< Default number of LRU caches.
    static const size_t DEFAULT_SHARECACHE_TOTAL_CHARGE; ///< Default total charge to be divided among caches.
};

const size_t ShareLRUConfig::DEFAULT_SHARECACHE_BIT = 4;
const size_t ShareLRUConfig::DEFAULT_SHARECACHE_N = 1 << ShareLRUConfig::DEFAULT_SHARECACHE_BIT;
const size_t ShareLRUConfig::DEFAULT_SHARECACHE_TOTAL_CHARGE = 4000;

/**
 * @class ShareLRUCache
 * @brief A shared LRU cache that divides a total charge across multiple LRU caches.
 * 
 * This class manages multiple LRU caches and ensures the cache is shared across them 
 * using a hash-based distribution. The cache is divided into a specified number of
 * individual LRU caches, each of which manages its own items and charges.
 *
 * @tparam Key Type of the key in the cache.
 * @tparam Value Type of the value in the cache.
 */
template <typename Value>
class ShareLRUCache {
    private :
    LRUCache<Value>* m_caches; ///< Array of LRUCache instances.
    size_t m_count; ///< Number of LRUCache instances.
    size_t total_charge_; ///< Total charge shared among caches.

     static uint32_t Shard(uint32_t hash) { return hash >> (32 - ShareLRUConfig::DEFAULT_SHARECACHE_BIT); }

    public :
    /**
     * @brief Constructs a shared LRU cache with a specified number of caches.
     * @param charge The total charge to divide among caches.
     */
    ShareLRUCache(size_t charge = LRUConfig::DEFAULT_CACHE_TOTAL_CHANGE) {
        assert(charge > 0);
        m_count = ShareLRUConfig::DEFAULT_SHARECACHE_N;
        m_caches = new LRUCache<Value>[m_count];
        for (size_t i = 0 ; i < m_count; i++) {
        m_caches[i].SetCharge((charge + m_count - 1) / m_count);
        }
        total_charge_ = charge;
    }  

    /**
     * @brief Destructor that cleans up the allocated memory.
     */
    ~ShareLRUCache() {
        delete [] m_caches;
    }

     /**
     * @brief Inserts a new key-value pair into the shared cache.
     * @param key The key to insert.
     * @param value The value to insert.
     * @param charge The charge (size) of the item to insert.
     * @return A pointer to the inserted cache node.
     */
    BaseNode* Insert(std::string key, Value value, size_t charge = LRUConfig::DEFAULT_CACHE_CHARGE_PER) {
        uint32_t hash = support::MurmurHash::Hash(key.data(),key.size(),0);
        return  (BaseNode *) m_caches[Shard(hash)].Insert(key, value, hash, charge);
    }

    /**
     * @brief Removes a key-value pair from the shared cache.
     * @param key The key to remove.
     */
    void Remove(std::string key) {
        uint32_t hash = support::MurmurHash::Hash(key.data(),key.size(),0);
        m_caches[Shard(hash)].Remove(key);
    } 

    /**
     * @brief Retrieves a value from the shared cache using a key.
     * @param key The key to search for.
     * @return A pointer to the cache node if found, nullptr otherwise.
     */
    BaseNode* Get(std::string key) {
        uint32_t hash = support::MurmurHash::Hash(key.data(),key.size(),0);
        return m_caches[Shard(hash)].Get(key);
    } 

     /**
     * @brief Calculates the total cache usage across all individual caches.
     * @return The total usage of the shared cache.
     */
    size_t TotalUsage() {
        size_t t = 0;
        for (size_t i = 0; i <m_count; i++) {
            t += m_caches[i].TotalUsage();
        }
        return t;
    }

    /**
     * @brief Prints the state of each individual LRU cache in the shared cache.
     */
    void Print() {
        for (size_t i = 0 ; i < m_count; i++) {
            std::cout <<"Cache: " << i + 1 << " =====" << std::endl;
            m_caches[i].Print();
            m_caches[i].Detail();
            m_caches[i].PrintInUsed();
            m_caches[i].PrintOutDated();
            std::cout << std::endl;
        }
    }

    /**
     * @brief Prints detailed information about the shared cache and its individual caches.
     */
    void Detail() {
        std::cout << "Total Charge: " << total_charge_ << std::endl; 
        std::cout << "Total Usage: " << TotalUsage() << std::endl; 
        for (size_t i = 0 ; i < m_count; i++) {
            std::cout <<"Cache: " << i + 1 << " =====" << std::endl;
            m_caches[i].Detail();
        }
    }

    /**
     * @brief Retrieves the LRU cache at a specified index.
     * @param index The index of the LRU cache to retrieve.
     * @return A pointer to the specified LRU cache.
     */
    LRUCache<Value>* GetLRU(size_t index) {
        return &m_caches[index];
    }

    /**
     * @brief Prunes all individual caches by removing outdated nodes.
     */
    void Prune() {
        for (size_t i = 0 ; i < m_count; i++) {
            m_caches[i].Prune();
        }
    }

     /**
     * @brief Releases a cache node, decreasing its reference count.
     * @param bnode The cache node to release.
     */
    BaseNode* Release(BaseNode* bnode) {
        if (bnode == nullptr) {
            return nullptr;
        }
        typename LRUCache<Value>::BucketValueNode* node = 
            static_cast<typename LRUCache<Value>::BucketValueNode*>(bnode);\
            uint32_t hash = support::MurmurHash::Hash(node->getKey().data(),node->getKey().size(),0);
            m_caches[Shard(hash)].Release(node);
        return bnode;
    }

    /**
     * @brief Retrieves the value stored in a cache node.
     * @param bnode The cache node.
     * @return The value of the cache item.
     */
    static Value& GetValue(BaseNode* bnode) {
        // if (bnode == nullptr) {
        //     return Value();
        // }
         typename LRUCache<Value>::BucketValueNode* node = 
            static_cast<typename LRUCache<Value>::BucketValueNode*>(bnode);
        return node->getValue();
    }

     /**
     * @brief Retrieves the key of a cache node.
     * @param bnode The cache node.
     * @return The key of the cache item.
     */
    static std::string GetKey(BaseNode* bnode) {
        if (bnode == nullptr) {
            return Value();
        }
         typename LRUCache<Value>::BucketValueNode* node = 
            static_cast<typename LRUCache<Value>::BucketValueNode*>(bnode);
        return node->getKey();
    }
};

} // namespace tagfilterdb

#endif