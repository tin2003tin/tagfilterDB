#include "leveldb/status.h"
#include "leveldb/env.h"
#include "leveldb/cache.h"
#include "util/coding.h"
#include "util/hash.h"
#include "util/arena.h"
#include "util/random.h"
#include "util/logging.h"
#include "util/random.h"
#include "util/crc32c.h"
#include <cassert>
#include <vector>
#include <iostream>

namespace leveldb {

static std::string EncodeKey(int k) {
  std::string result;
  PutFixed32(&result, k);
  return result;
}
static int DecodeKey(const Slice& k) {
  assert(k.size() == 4);
  return DecodeFixed32(k.data());
}
static void* EncodeValue(uintptr_t v) { return reinterpret_cast<void*>(v); }
static int DecodeValue(void* v) { return reinterpret_cast<uintptr_t>(v); }

class CacheTest {
 public:
  static void Deleter(const Slice& key, void* v) {
    current_->deleted_keys_.push_back(DecodeKey(key));
    current_->deleted_values_.push_back(DecodeValue(v));
  }

  static constexpr int kCacheSize = 1000;
  std::vector<int> deleted_keys_;
  std::vector<int> deleted_values_;
  Cache* cache_;

  CacheTest() : cache_(NewLRUCache(kCacheSize)) { current_ = this; }

  ~CacheTest() { delete cache_; }

  int Lookup(int key) {
    Cache::Handle* handle = cache_->Lookup(EncodeKey(key));
    const int r = (handle == nullptr) ? -1 : DecodeValue(cache_->Value(handle));
    if (handle != nullptr) {
      cache_->Release(handle);
    }
    return r;
  }

  void Insert(int key, int value, int charge = 1) {
    cache_->Release(cache_->Insert(EncodeKey(key), EncodeValue(value), charge,
                                   &CacheTest::Deleter));
  }

  Cache::Handle* InsertAndReturnHandle(int key, int value, int charge = 1) {
    return cache_->Insert(EncodeKey(key), EncodeValue(value), charge,
                          &CacheTest::Deleter);
  }

  void Erase(int key) { cache_->Erase(EncodeKey(key)); }
  static CacheTest* current_;
};
CacheTest* CacheTest::current_;
}

void testEncodingDecoding() {
  using namespace leveldb;
   std::string s;
  for (uint32_t i = 0; i < (32 * 32); i++) {
    uint32_t v = (i / 32) << (i % 32);
    PutVarint32(&s, v);
  }

  const char* p = s.data();
  const char* limit = p + s.size();
  for (uint32_t i = 0; i < (32 * 32); i++) {
    uint32_t expected = (i / 32) << (i % 32);
    uint32_t actual;
    const char* start = p;
    p = GetVarint32Ptr(p, limit, &actual);
    assert(p != nullptr);
    assert(expected == actual);
    assert(VarintLength(actual) == p - start);
  }
  assert(p == s.data() + s.size());

  std::cout << "[PASS] testEncodingDecoding" << std::endl;
}

void testCacheOperations() {
  using namespace leveldb;
  CacheTest c;
  assert(-1 == c.Lookup(100));

  c.Insert(100, 101);
  assert(101 == c.Lookup(100));
  assert(-1 == c.Lookup(200));
  assert(-1 == c.Lookup(300));

  c.Insert(200, 201);
  assert(101 == c.Lookup(100));
  assert(201 == c.Lookup(200));
  assert(-1 == c.Lookup(300));

  c.Insert(100, 102);
  assert(102 == c.Lookup(100));
  assert(201 == c.Lookup(200));
  assert(-1 == c.Lookup(300));

  assert(1 == c.deleted_keys_.size());
  assert(100 == c.deleted_keys_[0]);
  assert(101 == c.deleted_values_[0]);
    
  c.Erase(200);
  assert(2 == c.deleted_keys_.size());
  assert(200 == c.deleted_keys_[1]);
  assert(201 == c.deleted_values_[1]);
  c.Erase(100);
  assert(3 == c.deleted_keys_.size());
  assert(100 == c.deleted_keys_[2]);
  assert(102 == c.deleted_values_[2]);

  std::vector<Cache::Handle*> h;
  for (int i = 0; i < CacheTest::kCacheSize + 100; i++) {
    h.push_back(c.InsertAndReturnHandle(1000 + i, 2000 + i));
  }

  for (int i = 0; i < h.size(); i++) {
    assert(2000 + i == c.Lookup(1000 + i));
  }

  for (int i = 0; i < h.size(); i++) {
    c.cache_->Release(h[i]);
  }
  
  std::cout << "[PASS] testCacheOperations" << std::endl;
}

void testHashing() {
  using namespace leveldb;
  const uint8_t data1[1] = {0x62};
  const uint8_t data2[2] = {0xc3, 0x97};
  const uint8_t data3[3] = {0xe2, 0x99, 0xa5};
  const uint8_t data4[4] = {0xe1, 0x80, 0xb9, 0x32};
  const uint8_t data5[48] = {
      0x01, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x18, 0x28, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  assert(Hash(0, 0, 0xbc9f1d34) == 0xbc9f1d34);
  assert(
      Hash(reinterpret_cast<const char*>(data1), sizeof(data1), 0xbc9f1d34) ==
      0xef1345c4);
  assert(
      Hash(reinterpret_cast<const char*>(data2), sizeof(data2), 0xbc9f1d34) ==
      0x5b663814);
  assert(
      Hash(reinterpret_cast<const char*>(data3), sizeof(data3), 0xbc9f1d34) ==
      0x323c078f);
  assert(
      Hash(reinterpret_cast<const char*>(data4), sizeof(data4), 0xbc9f1d34) ==
      0xed21633a);
  assert(
      Hash(reinterpret_cast<const char*>(data5), sizeof(data5), 0x12345678) ==
      0xf333dabb);
    std::cout << "[PASS] testHashing" << std::endl;
}

void testStatus() {
  using namespace leveldb;
    Status ok = Status::OK();
    Status ok2 = std::move(ok);

    assert(ok2.ok());

    Status status = Status::NotFound("custom NotFound status message");
    Status status2 = std::move(status);

    assert(status2.IsNotFound());
    assert("NotFound: custom NotFound status message" == status2.ToString());

    Status self_moved = Status::IOError("custom IOError status message");

    Status& self_moved_reference = self_moved;
    self_moved_reference = std::move(self_moved);
    std::cout << "[PASS] testStatus" << std::endl;
}

void testArena() {
  using namespace leveldb;
  std::vector<std::pair<size_t, char*>> allocated;
  Arena arena;
  const int N = 100000;
  size_t bytes = 0;
  Random rnd(301);
  for (int i = 0; i < N; i++) {
    size_t s;
    if (i % (N / 10) == 0) {
      s = i;
    } else {
      s = rnd.OneIn(4000)
              ? rnd.Uniform(6000)
              : (rnd.OneIn(10) ? rnd.Uniform(100) : rnd.Uniform(20));
    }
    if (s == 0) {
      // Our arena disallows size 0 allocations.
      s = 1;
    }
    char* r;
    if (rnd.OneIn(10)) {
      r = arena.AllocateAligned(s);
    } else {
      r = arena.Allocate(s);
    }

    for (size_t b = 0; b < s; b++) {
      // Fill the "i"th allocation with a known bit pattern
      r[b] = i % 256;
    }
    bytes += s;
    allocated.push_back(std::make_pair(s, r));
    assert(arena.MemoryUsage() >= bytes);
    if (i > N / 10) {
      assert(arena.MemoryUsage() <= bytes * 1.10);
    }
  }
  for (size_t i = 0; i < allocated.size(); i++) {
    size_t num_bytes = allocated[i].first;
    const char* p = allocated[i].second;
    for (size_t b = 0; b < num_bytes; b++) {
      // Check the "i"th allocation for the known bit pattern
      assert((int)(int(p[b]) & 0xff) == (int)(i % 256));
    }
  }
   std::cout << "[PASS] testArena" << std::endl;
}

void testLogging() {
    assert(leveldb::NumberToString(0) == "0");
    assert("12345678" == leveldb::NumberToString(12345678));
    assert("18446744073709551614" == leveldb::NumberToString(18446744073709551614U));
    assert("18446744073709551615" == leveldb::NumberToString(18446744073709551615U));

    uint64_t number = 1000;
    const std::string& padding = "abc";
    std::string decimal_number = leveldb::NumberToString(number);
    std::string input_string = decimal_number + padding;
    leveldb::Slice input(input_string);
    leveldb::Slice output = input;
    uint64_t result;
    assert(ConsumeDecimalNumber(&output, &result));
    assert(number == result);
    assert(decimal_number.size() == output.data() - input.data());
    assert(padding.size() == output.size());

   std::cout << "[PASS] testLogging" << std::endl;
}

class EnvTest {
 public:
  EnvTest() : env_(leveldb::Env::Default()) {}

  leveldb::Env* env_;
};

leveldb::Slice RandomString(leveldb::Random* rnd, int len, std::string* dst) {
  dst->resize(len);
  for (int i = 0; i < len; i++) {
    (*dst)[i] = static_cast<char>(' ' + rnd->Uniform(95));  // ' ' .. '~'
  }
  return leveldb::Slice(*dst);
}

void testEnv() {
  leveldb::Random rnd(1);
    EnvTest envTest;

    std::string test_dir;
    assert(envTest.env_->GetTestDirectory(&test_dir).ok());
    std::string test_file_name = test_dir + "/open_on_read.txt";
    leveldb::WritableFile* writable_file;
    assert(envTest.env_->NewWritableFile(test_file_name, &writable_file).ok());
    static const size_t kDataSize = 10 * 1000;
    std::string data;
    while (data.size() < kDataSize) {
    int len = rnd.Skewed(18);  // Up to 2^18 - 1, but typically much smaller
    std::string r;
    RandomString(&rnd, len, &r);
    assert(writable_file->Append(r).ok());
    data += r;
    if (rnd.OneIn(10)) {
      assert(writable_file->Flush().ok());
    }
  }
  assert(writable_file->Sync().ok());
  assert(writable_file->Close().ok());
  delete writable_file;
   
  leveldb::SequentialFile* sequential_file;
  assert(envTest.env_->NewSequentialFile(test_file_name, &sequential_file).ok());
  std::string read_result;
  std::string scratch;
  while (read_result.size() < data.size()) {
    int len = std::min<int>(rnd.Skewed(18), data.size() - read_result.size());
    scratch.resize(std::max(len, 1));  // at least 1 so &scratch[0] is legal
    leveldb::Slice read;
    assert(sequential_file->Read(len, &read, &scratch[0]).ok());
    if (len > 0) {
      assert(read.size() >= 0);
    }
    assert(read.size() <= len);
    read_result.append(read.data(), read.size());
  }
  assert(read_result == data);
  delete sequential_file;

    
   std::cout << "[PASS]" << std::endl;
}
void testCRC32C() {
  // From rfc3720 section B.4.
  using namespace leveldb::crc32c;
  char buf[32];

  memset(buf, 0, sizeof(buf));
  assert(0x8a9136aa == Value(buf, sizeof(buf)));

  memset(buf, 0xff, sizeof(buf));
  assert(0x62a8ab43 == Value(buf, sizeof(buf)));

  for (int i = 0; i < 32; i++) {
    buf[i] = i;
  }
  assert(0x46dd794e == Value(buf, sizeof(buf)));

  for (int i = 0; i < 32; i++) {
    buf[i] = 31 - i;
  }
  assert(0x113fdb5c == Value(buf, sizeof(buf)));

  uint8_t data[48] = {
      0x01, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x18, 0x28, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  assert(0xd9963a56 == Value(reinterpret_cast<char*>(data), sizeof(data)));
}

int main() {
    std::cout << "Running tests..." << std::endl;

    testEncodingDecoding();
    testCacheOperations();
    testHashing();
    testStatus();
    testArena();
    testLogging();  
    testEnv();  
    testCRC32C();

    std::cout << "All tests passed!" << std::endl;

    return 0;
}
