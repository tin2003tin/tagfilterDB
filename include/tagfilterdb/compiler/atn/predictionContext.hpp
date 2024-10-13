#pragma once

#include <atomic>

#include "../recognizer.hpp"
#include "ATN.hpp"
#include "ATNState.hpp"
#include "predictionContextType.hpp"

namespace tagfilterdb::compiler::atn {
class PredictionContext {
  public:
    static const Ref<const PredictionContext> EMPTY;
    static constexpr size_t EMPTY_RETURN_STATE =
        std::numeric_limits<size_t>::max() - 9;

    static Ref<const PredictionContext>
    merge(Ref<const PredictionContext> a, Ref<const PredictionContext> b,
          bool rootIsWildcard, PredictionContextMergeCache *mergeCache);

    static Ref<const PredictionContext>
    mergeSingletons(Ref<const SingletonPredictionContext> a,
                    Ref<const SingletonPredictionContext> b,
                    bool rootIsWildcard,
                    PredictionContextMergeCache *mergeCache);

    static Ref<const PredictionContext>
    mergeRoot(Ref<const SingletonPredictionContext> a,
              Ref<const SingletonPredictionContext> b, bool rootIsWildcard);

    static Ref<const PredictionContext>
    mergeArrays(Ref<const ArrayPredictionContext> a,
                Ref<const ArrayPredictionContext> b, bool rootIsWildcard,
                PredictionContextMergeCache *mergeCache);

    static std::string toDOTString(const Ref<const PredictionContext> &context);

    static Ref<const PredictionContext>
    getCachedContext(const Ref<const PredictionContext> &context,
                     PredictionContextCache &contextCache);

    static std::vector<Ref<const PredictionContext>>
    getAllContextNodes(const Ref<const PredictionContext> &context);

    static Ref<const PredictionContext>
    fromRuleContext(const ATN &atn, RuleContext *outerContext);

    PredictionContext(const PredictionContext &) = delete;

    virtual ~PredictionContext() = default;

    PredictionContext &operator=(const PredictionContext &) = delete;
    PredictionContext &operator=(PredictionContext &&) = delete;

    PredictionContextType getContextType() const { return _contextType; }

    virtual size_t size() const = 0;
    virtual const Ref<const PredictionContext> &
    getParent(size_t index) const = 0;
    virtual size_t getReturnState(size_t index) const = 0;

    virtual bool isEmpty() const = 0;
    bool hasEmptyPath() const;

    size_t hashCode() const;

    virtual bool equals(const PredictionContext &other) const = 0;

    virtual std::string toString() const = 0;

    std::vector<std::string> toStrings(Recognizer *recognizer,
                                       int currentState) const;
    std::vector<std::string> toStrings(Recognizer *recognizer,
                                       const Ref<const PredictionContext> &stop,
                                       int currentState) const;

  protected:
    explicit PredictionContext(PredictionContextType contextType);

    PredictionContext(PredictionContext &&other);

    virtual size_t hashCodeImpl() const = 0;

    size_t cachedHashCode() const {
        return _hashCode.load(std::memory_order_relaxed);
    }

  private:
    const PredictionContextType _contextType;
    mutable std::atomic<size_t> _hashCode;
};
} // namespace tagfilterdb::compiler::atn

namespace std {

template <> struct hash<tagfilterdb::compiler::atn::PredictionContext> {
    size_t operator()(const tagfilterdb::compiler::atn::PredictionContext
                          &predictionContext) const {
        return predictionContext.hashCode();
    }
};

} // namespace std

#include "predictionContext.cpp"