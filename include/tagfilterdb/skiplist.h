/**
 * @file skiplist.h
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


#ifndef TAGFILERDB_SKIPLIST_H
#define TAGFILERDB_SKIPLIST_H

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <mutex>
#include <shared_mutex>

#include "arena.h"
#include "random.h"

#define SKIPLIST_TEMPLATE template <typename Key, typename Value, class Comparator>
#define SKIPLIST_CLASS SkipList<Key, Value, Comparator>

namespace tagfilterdb {

SKIPLIST_TEMPLATE
class SkipList {
 private:
  struct Node;

 public:

  explicit SkipList(Comparator cmp, Arena* arena);

  void Insert(const Key& key, const Value& value);

  bool Contains(const Key& key) const;

  bool Get(const Key& key, Value* value) const;
  
  Value& Get(const Key& key) const;
  
  void Remove(const Key& key);

  class Iterator {
   public:

    explicit Iterator(const SkipList* list);

    bool Valid() const;

    const Key& key()  const;

    Value& value();

    void Next();

    void Prev();

    void Seek(const Key& target);

    void SeekToFirst();

    void SeekToLast();

   private:
    const SkipList* list_;
    Node* node_;
  };

 private:
  enum { kMaxHeight = 12 };

  inline int GetMaxHeight() const {
    return max_height_.load(std::memory_order_relaxed);
  }

  Node* NewNode(const Key& key, const Value& value, int height);

  int RandomHeight();

  bool Equal(const Key& a, const Key& b) const { return (compare_(a, b) == 0); }

  bool KeyIsAfterNode(const Key& key, Node* n) const;

  Node* FindGreaterOrEqual(const Key& key, Node** prev) const;

  Node* FindLessThan(const Key& key) const;

  Node* FindLast() const;

  Comparator const compare_;
  Arena* const arena_;  

  Node* const head_;

  std::atomic<int> max_height_;  

  Random rnd_;

  mutable std::shared_mutex mutex_; 
};

// Implementation details follow
SKIPLIST_TEMPLATE
struct SKIPLIST_CLASS::Node {
  
  explicit Node(const Key& key, const Value& value) 
      : key_(key), value_(value) {}

  Key const key_;
  Value value_;

  Node* Next(int n) {
    assert(n >= 0);
    // Use an 'acquire load' so that we observe a fully initialized
    // version of the returned Node.
    return next_[n].load(std::memory_order_acquire);
  }
  
  void SetNext(int n, Node* x) {
    assert(n >= 0);
    // Use a 'release store' so that anybody who reads through this
    // pointer observes a fully initialized version of the inserted node.
    next_[n].store(x, std::memory_order_release);
  }

  // No-barrier variants that can be safely used in a few locations.
  Node* NoBarrier_Next(int n) {
    assert(n >= 0);
    return next_[n].load(std::memory_order_relaxed);
  }

  void NoBarrier_SetNext(int n, Node* x) {
    assert(n >= 0);
    next_[n].store(x, std::memory_order_relaxed);
  }

 private:
  // Array of length equal to the node height.  next_[0] is lowest level link.
  std::atomic<Node*> next_[1];
};

SKIPLIST_TEMPLATE
typename SKIPLIST_CLASS::Node* SKIPLIST_CLASS::NewNode(
    const Key& key, const Value& value, int height) {
  char* const node_memory = arena_->AllocateAligned(
      sizeof(Node) + sizeof(std::atomic<Node*>) * (height - 1));
   return new (node_memory) Node(key, value);
}

SKIPLIST_TEMPLATE
inline SKIPLIST_CLASS::Iterator::Iterator(const SkipList* list) {
  list_ = list;
  node_ = nullptr;
}

SKIPLIST_TEMPLATE
inline bool SKIPLIST_CLASS::Iterator::Valid() const {
  return node_ != nullptr;
}

SKIPLIST_TEMPLATE
inline const Key& SKIPLIST_CLASS::Iterator::key() const {
  assert(Valid());
  return node_->key_;
}

SKIPLIST_TEMPLATE
inline Value& SKIPLIST_CLASS::Iterator::value() {
  assert(Valid());
  return node_->value_;
}

SKIPLIST_TEMPLATE
inline void SKIPLIST_CLASS::Iterator::Next() {
  assert(Valid());
  node_ = node_->Next(0);
}

SKIPLIST_TEMPLATE
inline void SKIPLIST_CLASS::Iterator::Prev() {
  // Instead of using explicit "prev" links, we just search for the
  // last node that falls before key.
  assert(Valid());
  node_ = list_->FindLessThan(node_->key);
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

SKIPLIST_TEMPLATE
inline void SKIPLIST_CLASS::Iterator::Seek(const Key& target) {
  node_ = list_->FindGreaterOrEqual(target, nullptr);
}

SKIPLIST_TEMPLATE
inline void SKIPLIST_CLASS::Iterator::SeekToFirst() {
  node_ = list_->head_->Next(0);
}

SKIPLIST_TEMPLATE
inline void SKIPLIST_CLASS::Iterator::SeekToLast() {
  node_ = list_->FindLast();
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

SKIPLIST_TEMPLATE
int SKIPLIST_CLASS::RandomHeight() {
  // Increase height with probability 1 in kBranching
  static const unsigned int kBranching = 4;
  int height = 1;
  while (height < kMaxHeight && rnd_.OneIn(kBranching)) {
    height++;
  }
  assert(height > 0);
  assert(height <= kMaxHeight);
  return height;
}

SKIPLIST_TEMPLATE
bool SKIPLIST_CLASS::KeyIsAfterNode(const Key& key, Node* n) const {
  // null n is considered infinite
  return (n != nullptr) && (compare_(n->key_, key) < 0);
}

SKIPLIST_TEMPLATE
typename SKIPLIST_CLASS::Node*
SKIPLIST_CLASS::FindGreaterOrEqual(const Key& key,
                                              Node** prev) const {
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    Node* next = x->Next(level);
    if (KeyIsAfterNode(key, next)) {
      // Keep searching in this list
      x = next;
    } else {
      if (prev != nullptr) prev[level] = x;
      if (level == 0) {
        return next;
      } else {
        // Switch to next list
        level--;
      }
    }
  }
}

SKIPLIST_TEMPLATE
typename SKIPLIST_CLASS::Node*
SKIPLIST_CLASS::FindLessThan(const Key& key) const {
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    assert(x == head_ || compare_(x->key, key) < 0);
    Node* next = x->Next(level);
    if (next == nullptr || compare_(next->key, key) >= 0) {
      if (level == 0) {
        return x;
      } else {
        // Switch to next list
        level--;
      }
    } else {
      x = next;
    }
  }
}

SKIPLIST_TEMPLATE
typename SKIPLIST_CLASS::Node* SKIPLIST_CLASS::FindLast()
    const {
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    Node* next = x->Next(level);
    if (next == nullptr) {
      if (level == 0) {
        return x;
      } else {
        // Switch to next list
        level--;
      }
    } else {
      x = next;
    }
  }
}

SKIPLIST_TEMPLATE
SKIPLIST_CLASS::SkipList(Comparator cmp, Arena* arena)
    : compare_(cmp),
      arena_(arena),
      head_(NewNode(Key(), Value(), kMaxHeight)),
      max_height_(1),
      rnd_(0xdeadbeef) {
  for (int i = 0; i < kMaxHeight; i++) {
    head_->SetNext(i, nullptr);
  }
}

SKIPLIST_TEMPLATE
void SKIPLIST_CLASS::Insert(const Key& key, const Value& value)  {
  std::unique_lock lock(mutex_);
  Node* prev[kMaxHeight];
  Node* x = FindGreaterOrEqual(key, prev);

  // Our data structure does not allow duplicate insertion
  assert(x == nullptr || !Equal(key, x->key_));

  int height = RandomHeight();
  if (height > GetMaxHeight()) {
    for (int i = GetMaxHeight(); i < height; i++) {
      prev[i] = head_;  
    }

    max_height_.store(height, std::memory_order_relaxed);
  }

  x = NewNode(key, value, height);
  for (int i = 0; i < height; i++) {
    // NoBarrier_SetNext() suffices since we will add a barrier when
    // we publish a pointer to "x" in prev[i].
    x->NoBarrier_SetNext(i, prev[i]->NoBarrier_Next(i));
    prev[i]->SetNext(i, x);
  }
}

SKIPLIST_TEMPLATE
bool SKIPLIST_CLASS::Contains(const Key& key) const {
  std::shared_lock lock(mutex_);
  Node* x = FindGreaterOrEqual(key, nullptr);
  if (x != nullptr && Equal(key, x->key_)) {
    return true;
  } else {
    return false;
  }
}

SKIPLIST_TEMPLATE
bool SKIPLIST_CLASS::Get(const Key& key, Value* value) const {
  std::shared_lock lock(mutex_);
  Node* x = FindGreaterOrEqual(key, nullptr);
  if (x != nullptr && Equal(key, x->key_)) {
    *value = x->value_;  // If the key exists, retrieve its value
    return true;
  } else {
    return false;  // If the key is not found, return false
  }
}

SKIPLIST_TEMPLATE
Value& SKIPLIST_CLASS::Get(const Key& key) const {
  std::shared_lock lock(mutex_);
  Node* x = FindGreaterOrEqual(key, nullptr);
  if (x != nullptr && Equal(key, x->key_)) {
    return x->value_;
  } else {
    Value v;
    return v;
  }
}

SKIPLIST_TEMPLATE
void SKIPLIST_CLASS::Remove(const Key& key) {
  std::unique_lock lock(mutex_);
  Node* prev[kMaxHeight];
  Node* x = FindGreaterOrEqual(key, prev);

  // If the key is not found, there's nothing to remove
  if (x == nullptr || !Equal(key, x->key_)) {
    return;
  }

  // Update the pointers of the previous nodes to bypass the node to be removed
  for (int i = 0; i < GetMaxHeight(); i++) {
    if (prev[i]->Next(i) != x) {
      break;
    }
    prev[i]->SetNext(i, x->Next(i));
  }

  // Deallocate the node
  delete x;  
}

}  

#endif  // STORAGE_LEVELDB_DB_SKIPLIST_H_