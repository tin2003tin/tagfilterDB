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
 * @tparam Key Type of the key in the cache.
 * @tparam Value Type of the value in the cache.
 */
template <typename Key, typename Value>
class LRUCache {
    public :

    /**
     * @class CacheMux
     * @brief A class to handle locking mechanisms for cache access.
     */
    class CacheMux {
        protected:
        std::mutex* _mux;
        CacheMux(std::mutex* mux) {
            this->_mux = mux;
            _mux->lock();
        }
        ~CacheMux() {
            _mux->unlock();
        }

        friend LRUCache;
    };

     /**
     * @class BucketNode
     * @brief A class representing a node in the bucket list.
     */
    class BucketNode {
        protected:
        BucketNode* m_next; ///< Pointer to the next node in the bucket.
        BucketNode* m_lNext; ///< Pointer to the next node in the LRU list.
        BucketNode* m_lPrev; ///< Pointer to the previous node in the LRU list.

        BucketNode() : m_next(nullptr), m_lNext(nullptr), m_lPrev(nullptr) {}
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
        size_t m_ref; ///< Reference count for the node.

        Referenceable() : m_ref(1) {}

        friend LRUCache;
    };
    
    /**
     * @class BucketValueNode
     * @brief A class representing a value node in the cache.
     */
    class BucketValueNode final : public BucketNode, public Referenceable, public BaseNode {
        protected:
        Key m_key; ///< The key of the cache item.
        Value m_value; ///< The value of the cache item.
        size_t m_charge; ///< The charge (size) of the cache item.

        BucketValueNode(Key key, Value value, size_t charge) : 
        m_key(key), m_value(value), m_charge(charge) {
    }       
        public:
        /**
         * @brief Returns the value stored in the cache node.
         * @return The value of the cache item.
         */
        Value getValue() {
            assert(this);
            return m_value; 
        }

         /**
         * @brief Returns the key of the cache node.
         * @return The key of the cache item.
         */
        Key getKey() {
            assert(this);
            return m_key;
        }

        friend LRUCache;
    };
    private: 
    BucketNode* m_data; ///< Array of bucket nodes to store the cache data.
    size_t m_cap; ///< The capacity of the cache.
    size_t m_size; ///< The current size of the cache.
    size_t m_total_charge; ///< The total charge available for the cache.
    size_t m_total_usage; ///< The total usage of cache space.

    BucketNode* m_inUsed_head; ///< Head of the LRU "in-use" list.
    BucketNode* m_inUsed_tail; ///< Tail of the LRU "in-use" list.

    BucketNode* m_outdated_head; ///< Head of the LRU "outdated" list.
    BucketNode* m_outdated_tail; ///< Tail of the LRU "outdated" list.
    
    std::mutex m_mux; ///< Mutex used to synchronize cache access.

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
        for (size_t i = 0 ; i < m_cap; i++) {
            BucketNode* curr = m_data[i].m_next;
            while (curr != nullptr) {
                BucketNode* next = curr->m_next;
                delete curr;
                curr = next;
            }
        }
        delete []m_data;
        delete m_outdated_head;
        delete m_outdated_tail;
        delete m_inUsed_head;
        delete m_inUsed_tail;
    } 

    /**
     * @brief Sets the charge for the cache.
     * @param charge The new total charge for the cache.
     */
    void SetCharge(size_t charge) {
        m_total_charge = charge;
    }

    /**
     * @brief Inserts a new key-value pair into the cache.
     * @param key The key to insert.
     * @param value The value to insert.
     * @param charge The charge (size) of the item to insert.
     * @return A pointer to the inserted cache node.
     */
    BucketValueNode* Insert(Key key, Value value, size_t charge = LRUConfig::DEFAULT_CACHE_CHARGE_PER) {
        if (charge > m_total_charge) {
            return nullptr;
        }
        assert(charge > 0);

        CacheMux m(&m_mux);

        if (isExpand()) {
            expand(m_cap * LRUConfig::DEFAULT_CAHCE_EXPAND);
        }

        std::hash<Key> h;
        size_t u = h(key);
        BucketValueNode* newNode =  new BucketValueNode(key,value,charge);

        size_t index = u % m_cap;
        BucketNode* prev = &m_data[index];

        while (prev->m_next != nullptr)
        {
            if (((BucketValueNode* )(prev->m_next))->m_key == key) {
                break;
            }
            prev = prev->m_next;
        }
        if (prev->m_next == nullptr) {
            prev->m_next = newNode;
            m_size++;
        } else {
            // The key already existed
            newNode->m_next = prev->m_next->m_next;

            removeList(prev->m_next);
            m_total_usage -= ((BucketValueNode* )(prev->m_next))->m_charge;
            delete prev->m_next;

            prev->m_next = newNode;
        }

        // Remove in OutDated List if excess a total charge
        while (m_total_usage + charge > m_total_charge 
               && m_outdated_tail->m_lPrev != m_outdated_head) {
            bool s = removeNode(((BucketValueNode* )(m_outdated_head->m_lNext))->m_key);
            assert(s);
        }

        appendToList(newNode, m_inUsed_tail);
        m_total_usage += charge;

        assert(m_total_usage > 0);
        assert(m_total_usage <= m_total_charge);
        ref(newNode);
        return newNode;
    }

    /**
     * @brief Removes a key-value pair from the cache.
     * @param key The key to remove.
     * @return True if the item was successfully removed, false otherwise.
     */
    bool Remove(Key key) {
       CacheMux m(&m_mux);
       return removeNode(key);
    }

    /**
     * @brief Retrieves a value from the cache using a key.
     * @param key The key to search for.
     * @return A pointer to the cache node if found, nullptr otherwise.
     */
    BucketValueNode* Get(Key key) {
        CacheMux m(&m_mux);

        std::hash<Key> h;
        size_t u = h(key) % m_cap;
        BucketValueNode* curr = (BucketValueNode*)  m_data[u].m_next;
        while (curr != nullptr) {
            if (curr->m_key == key) {
                ref(curr);
                return curr;
            }
            curr = (BucketValueNode*) curr->m_next;
        }
        return nullptr;
    }

    /**
     * @brief Prunes the cache by removing all outdated nodes.
     */
    void Prune() {
        BucketNode* curr = m_outdated_head->m_lNext;
        while (curr != m_outdated_tail) {
            BucketNode* next = curr->m_lNext;
            assert((((BucketValueNode *) curr)->m_ref == 1));
            bool s = removeNode(((BucketValueNode *)curr)->m_key);
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
        for (size_t i = 0; i < m_cap; i++) {
            std::cout << i << " ";
            BucketValueNode* curr = (BucketValueNode*)  m_data[i].m_next;
            while (curr != nullptr)
            {
                std::cout << "("<< curr->m_key << ", " << curr->m_value << ", " << curr->m_charge<< ", " << curr->m_ref <<  ") ";
                curr =  (BucketValueNode*) curr->m_next;
            }
            std::cout << std::endl;
        }
    }

     /**
     * @brief Returns the total charge of the cache.
     * @return The total charge.
     */
    size_t TotalCharge() const {
        return m_total_charge;
    }

     /**
     * @brief Returns the total usage of the cache.
     * @return The total usage.
     */
    size_t TotalUsage() const {
        return m_total_usage;
    }

     /**
     * @brief Prints the nodes in the "outdated" list.
     */
    void PrintOutDated() const {
        BucketNode* curr = m_outdated_head->m_lNext;
        std::cout << "OutDated: ";
        while (curr != m_outdated_tail)
        {
            BucketValueNode* node = ( BucketValueNode* ) curr;
            std::cout << "(" << node->m_key << ", "  << node->m_value << ") ";
            curr = curr->m_lNext;
        }
        std::cout << std::endl;
    }

    /**
     * @brief Prints the nodes in the "in-use" list.
     */
     void PrintInUsed() const {
        BucketNode* curr = m_inUsed_head->m_lNext;
        std::cout << "InUsed: ";
        while (curr != m_inUsed_tail)
        {
            BucketValueNode* node = ( BucketValueNode* ) curr;
            std::cout << "(" << node->m_key << ", "  << node->m_value << ") ";
            curr = curr->m_lNext;
        }
        std::cout << std::endl;
    }

     /**
     * @brief Prints detailed information about the cache.
     */
    void Detail() const {
        std::cout << "Detail:" << std::endl;
        std::cout << "- Capacity: " << m_cap << std::endl;
        std::cout << "- Size: " << m_size << std::endl;
        std::cout << "- Total Charge: " << m_total_charge << std::endl;
        std::cout << "- Total Usage: " << m_total_usage << std::endl;
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
        return ((BucketValueNode *) node)->m_value;
    }

    /**
     * @brief Retrieves the key of a cache node.
     * @param node The cache node.
     * @return The key of the cache item.
     */
    static Key GetKey(BucketValueNode* node) {
        if (node == nullptr) {
            return Key();
        }
        return node->m_key;
    }


    private:
        void setup(size_t cap, size_t total_charge) {
            assert(cap > 0);
            assert(total_charge > 0);

            m_data = new BucketNode[cap];
            m_cap = cap;
            m_size = 0;
            m_total_charge = total_charge;
            m_total_usage = 0;

            m_outdated_head = new BucketNode;
            m_outdated_tail = new BucketNode;
            m_outdated_head->m_lNext = m_outdated_tail;
            m_outdated_head->m_lPrev = m_outdated_tail;
            m_outdated_tail->m_lNext = m_outdated_head;
            m_outdated_tail->m_lPrev = m_outdated_head;

            m_inUsed_head = new BucketNode;
            m_inUsed_tail = new BucketNode;
            m_inUsed_head->m_lNext = m_inUsed_tail;
            m_inUsed_head->m_lPrev = m_inUsed_tail;
            m_inUsed_tail->m_lNext = m_inUsed_head;
            m_inUsed_tail->m_lPrev = m_inUsed_head;
        }

        void ref(BucketValueNode *refNode) {
            assert(refNode->m_ref >= 1);
            refNode->m_ref++;
            if (refNode->m_ref == 2) {
                 removeList(refNode);
                appendToList(refNode, m_inUsed_tail);
            }
        }

        void unref(BucketValueNode *refNode) {
            assert(refNode->m_ref >= 1);
            refNode->m_ref--;
            if (refNode->m_ref == 1) {
                removeList(refNode);
                appendToList(refNode, m_outdated_tail);
            } else if (refNode->m_ref == 0) {
                removeNode(refNode->m_key);
            }
        }

        bool isExpand() {
            return m_cap * LRUConfig::DEFAULT_CACHE_RATIO <  m_size;
        }

        bool isBucketValueNode(BucketNode* node) {
            return dynamic_cast<BucketValueNode*>(node) != nullptr;
        }

        void expand(size_t newCap) {
            BucketNode* newLoc = new BucketNode[newCap];
            std::hash<Key> h;     
            // rehash
            for (size_t i = 0 ; i < m_cap; i++) {
                BucketValueNode* curr = (BucketValueNode*) m_data[i].m_next;
                while (curr != nullptr) {
                    BucketValueNode* next = (BucketValueNode*)  curr->m_next;
                    curr->m_next = nullptr;
                    size_t u = h(curr->m_key) % newCap;  
                    if (newLoc[u].m_next == nullptr) {
                        newLoc[u].m_next = curr;
                    } else {
                        BucketNode* b = newLoc[u].m_next;
                        while (b->m_next != nullptr)
                        {
                            b= b->m_next;
                        }
                        b->m_next = curr;
                    }
                    curr = next;
                }
            }
            delete[] m_data;
            m_data = newLoc;
            m_cap = newCap;
        }

        bool removeNode(Key key) {
            std::hash<Key> h;
            size_t u = h(key) % m_cap;
            BucketValueNode* curr = (BucketValueNode*)  m_data[u].m_next;
            BucketNode* prev = &m_data[u];
            while (curr != nullptr && curr->m_key != key) {
                    prev = curr;
                    curr = (BucketValueNode*)  curr->m_next;
            } 
            if (curr == nullptr) {
                return false;
            }
            m_total_usage -= curr->m_charge; 
            m_size--;
            prev->m_next = curr->m_next;
            removeList(curr);
            delete curr;
            return true;
        }

        void appendToList(BucketNode* node, BucketNode* tail) {
            assert(node != nullptr);
            BucketNode* prev = tail->m_lPrev;
            node->m_lNext = tail;
            node->m_lPrev = prev;
            prev->m_lNext = node;
            tail->m_lPrev = node;
        }

        void removeList(BucketNode* node) {
            assert(node != nullptr);
            if (node->m_lNext == nullptr && node->m_lPrev == nullptr) {
                return;
            }
            BucketNode* next = node->m_lNext;                
            BucketNode* prev = node->m_lPrev;
            prev->m_lNext = next;
            next->m_lPrev = prev;

            node->m_lNext = nullptr;
            node->m_lPrev = nullptr;
        }
};

/**
 * @class ShareLRUConfig
 * @brief A configuration class to hold constants related to the shared LRU cache.
 */
class ShareLRUConfig {
public:
    static const size_t DEFAULT_SHARECACHE_N; ///< Default number of LRU caches.
    static const size_t DEFAULT_SHARECACHE_TOTAL_CHARGE; ///< Default total charge to be divided among caches.
};

const size_t ShareLRUConfig::DEFAULT_SHARECACHE_N = 4;
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
template <typename Key, typename Value>
class ShareLRUCache {
    LRUCache<Key, Value>* m_caches; ///< Array of LRUCache instances.
    size_t m_count; ///< Number of LRUCache instances.
    size_t m_total_charge; ///< Total charge shared among caches.

    public :
    /**
     * @brief Constructs a shared LRU cache with a specified number of caches.
     * @param n The number of LRUCache instances to create.
     * @param charge The total charge to divide among caches.
     */
    ShareLRUCache(size_t n = ShareLRUConfig::DEFAULT_SHARECACHE_N,
                 size_t charge = LRUConfig::DEFAULT_CACHE_TOTAL_CHANGE) {
        assert(n > 0);
        assert(charge > 0);
        m_caches = new LRUCache<Key,Value>[n];
        for (size_t i = 0 ; i < n; i++) {
        m_caches[i].SetCharge((charge + n - 1) / n);
        }
        m_count = n;
        m_total_charge = charge;
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
    BaseNode* Insert(Key key, Value value, size_t charge = LRUConfig::DEFAULT_CACHE_CHARGE_PER) {
        std::hash<Key> h;
        size_t index = h(key) % m_count;
        return  (BaseNode *) m_caches[index].Insert(key, value, charge);
    }

    /**
     * @brief Removes a key-value pair from the shared cache.
     * @param key The key to remove.
     */
    void Remove(Key key) {
        std::hash<Key> h;
        size_t index = h(key) % m_count;
        m_caches[index].Remove(key);
    } 

    /**
     * @brief Retrieves a value from the shared cache using a key.
     * @param key The key to search for.
     * @return A pointer to the cache node if found, nullptr otherwise.
     */
    BaseNode* Get(Key key) {
        std::hash<Key> h;
        size_t index = h(key) % m_count;
        return m_caches[index].Get(key);
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
        std::cout << "Total Charge: " << m_total_charge << std::endl; 
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
    LRUCache<Key,Value>* GetLRU(size_t index) {
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
        typename LRUCache<Key, Value>::BucketValueNode* node = 
            static_cast<typename LRUCache<Key, Value>::BucketValueNode*>(bnode);
        std::hash<Key> h;
        size_t u = h(node->getKey()) % m_count;
        m_caches[u].Release(node);
        return bnode;
    }

    /**
     * @brief Retrieves the value stored in a cache node.
     * @param bnode The cache node.
     * @return The value of the cache item.
     */
    static Value GetValue(BaseNode* bnode) {
        if (bnode == nullptr) {
            return Value();
        }
         typename LRUCache<Key, Value>::BucketValueNode* node = 
            static_cast<typename LRUCache<Key, Value>::BucketValueNode*>(bnode);
        return node->getValue();
    }

     /**
     * @brief Retrieves the key of a cache node.
     * @param bnode The cache node.
     * @return The key of the cache item.
     */
    static Key GetKey(BaseNode* bnode) {
        if (bnode == nullptr) {
            return Value();
        }
         typename LRUCache<Key, Value>::BucketValueNode* node = 
            static_cast<typename LRUCache<Key, Value>::BucketValueNode*>(bnode);
        return node->getKey();
    }
};

} // namespace tagfilterdb

#endif