#pragma once

#include "ATN.hpp"
#include "predictionContext.hpp"
#include "tagfilterdb/support/rangeSet.hpp"

namespace tagfilterdb::compiler::atn {
class ATNSimulator {
  public:
    const ATN &atn;
    ATNSimulator(const ATN &atn, PredictionContextCache &sharedContextCache);

    //     virtual ~ATNSimulator() = default;

    //     virtual void reset() = 0;

    //     virtual void clearDFA();

    //     PredictionContextCache &getSharedContextCache() const;
    //     Ref<const PredictionContext>
    //     getCachedContext(const Ref<const PredictionContext> &context);

    //   protected:
    //     PredictionContextCache &_sharedContextCache;
};
} // namespace tagfilterdb::compiler::atn