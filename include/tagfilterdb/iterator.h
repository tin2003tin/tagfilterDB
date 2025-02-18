#ifndef TAGFILTER_ITERATOR_H
#define TAGFILTER_ITERATOR_H

#include "dataView.h"
#include "status.h"
#include <cassert>

namespace tagfilterdb {
class Iterator {
  public:
    Iterator() {
        cleanup_head_.function = nullptr;
        cleanup_head_.next = nullptr;
    }

    Iterator(const Iterator &) = delete;
    Iterator &operator=(const Iterator &) = delete;

    virtual ~Iterator() {
        if (!cleanup_head_.Empty()) {
            cleanup_head_.Execute();
            CleanupNode *node = cleanup_head_.next;
            while (node != nullptr) {
                node->Execute();
                CleanupNode *next = node->next;
                delete node;
                node = next;
            }
        }
    }

    virtual bool Valid() const = 0;
    virtual void SeekToFirst() = 0;
    virtual void SeekToLast() = 0;
    virtual void Seek(const DataView &target) = 0;
    virtual void Next() = 0;
    virtual void Prev() = 0;
    virtual DataView GetKey() const = 0;
    virtual DataView GetValue() const = 0;
    virtual Status GetStatus() const = 0;

    using CleanupFunction = void (*)(void *arg1, void *arg2);
    void RegisterCleanup(CleanupFunction function, void *arg1, void *arg2) {
        assert(function != nullptr);
        CleanupNode *node;
        if (cleanup_head_.Empty()) {
            node = &cleanup_head_;
        } else {
            node = new CleanupNode;
            node->next = cleanup_head_.next;
            cleanup_head_.next = node;
        }
        node->function = function;
        node->arg1 = arg1;
        node->arg2 = arg2;
    }

  private:
    struct CleanupNode {

        bool Empty() const { return function == nullptr; }

        void Execute() {
            assert(function != nullptr);
            (*function)(arg1, arg2);
        }

        CleanupNode *next;
        CleanupFunction function;
        void *arg1;
        void *arg2;
    };
    CleanupNode cleanup_head_;
};

class EmptyIterator : public Iterator {
  public:
    EmptyIterator(const Status &s) : status_(s) {}

    ~EmptyIterator() override = default;

    bool Valid() const override { return false; }

    void SeekToFirst() override {}

    void SeekToLast() override {}

    void Seek(const DataView &target) override {}

    void Next() override {}

    void Prev() override {}

    DataView GetKey() const override {
        assert(false);
        return DataView();
    }

    DataView GetValue() const override {
        assert(false);
        return DataView();
    }

    Status GetStatus() const override { return status_; }

  private:
    Status status_;
};

EmptyIterator *NewEmptyIterator(const Status &s) {
    return new EmptyIterator(s);
}

} // namespace tagfilterdb

#endif