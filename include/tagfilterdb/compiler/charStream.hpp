#pragma once

#include "intStream.hpp"
#include "tagfilterdb/support/range.hpp"

namespace tagfilterdb::compiler {
class CharStream : public IntStream {
  public:
    virtual ~CharStream() = default;
    virtual std::string getText(const support::Range &range) = 0;
    virtual std::string toString() const = 0;
};
} // namespace tagfilterdb::compiler
