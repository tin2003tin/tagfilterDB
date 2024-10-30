#pragma once

#include "tagfilterdb/support/rangeSet.hpp"
#include "transitionType.hpp"

namespace tagfilterdb::compiler::atn {
class Transition {
  public:
    ATNState *target;
    virtual ~Transition() = default;
    TransitionType getTransitionType() const { return _transitionType; }
    virtual bool isEpsilon() const { return false; }
    virtual support::RangeSet label() const {
        return support::RangeSet::EMPTY_SET;
    }
    virtual bool matches(size_t symbol, size_t minVocabSymbol,
                         size_t maxVocabSymbol) const = 0;
    virtual std::string toString() const {
        std::stringstream ss;
        ss << "(Transition " << std::hex << this << ", target: " << std::hex
           << target << ')';

        return ss.str();
    }

    Transition(Transition const &) = delete;
    Transition &operator=(Transition const &) = delete;

  protected:
    Transition(TransitionType transitionType, ATNState *target)
        : _transitionType(transitionType) {
        if (target == nullptr) {
            throw NullPointerException("target cannot be null.");
        }

        this->target = target;
    }

  private:
    const TransitionType _transitionType;
};

using ConstTransitionPtr = std::unique_ptr<const Transition>;
} // namespace tagfilterdb::compiler::atn