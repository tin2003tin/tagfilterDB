#pragma once

#include "tagfilterdb/compiler/internalCommon.hpp"

namespace tagfilterdb::compiler {
class IntStream {
  public:
    static constexpr size_t EOF = std::numeric_limits<size_t>::max();
    static const std::string UNKNOWN_SOURCE_NAME;
    virtual ~IntStream();
    virtual void consume() = 0;
    virtual size_t LA(ssize_t i) = 0;
    virtual ssize_t mark() = 0;
    virtual void release(ssize_t marker) = 0;
    virtual size_t index() = 0;
    virtual void seek(size_t index) = 0;
    virtual size_t size() = 0;
    virtual std::string getSourceName() const = 0;
};
} // namespace tagfilterdb::compiler

#include "intStream.cpp"