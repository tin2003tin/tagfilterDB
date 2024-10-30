#ifndef TAGFILTER_DB_HPP
#define TAGFILTER_DB_HPP

#include "leveldb/cache.h"

#define DEFAULT_CACHE 1000

class TAGCache {
    leveldb::Cache* _c;
    TAGCache() : _c(leveldb::NewLRUCache((size_t) DEFAULT_CACHE)) {}
    leveldb::Cache::Handle* Insert(const leveldb::Slice &key, void *value, size_t charge, void (*deleter)(const leveldb::Slice &key, void *value)) {
        return _c->Insert(key,value,charge,deleter);
    }
    void Release(leveldb::Cache::Handle *handle) {
        _c->Release(handle);
    }
    leveldb::Cache::Handle *Lookup(const leveldb::Slice &key) {
        _c->Lookup(key);
    }
    void Erase(const leveldb::Slice &key) {
        _c->Erase(key);
    }
    size_t TotalCharge() const {
        return _c->TotalCharge();
    }

};

#endif