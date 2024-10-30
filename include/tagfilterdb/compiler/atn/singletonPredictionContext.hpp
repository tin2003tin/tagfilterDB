#pragma once

#include "predictionContext.hpp"

namespace tagfilterdb::compiler::atn {
class SingletonPredictionContext final : public PredictionContext {
  public:
    static bool is(const PredictionContext &predictionContext) {
        return predictionContext.getContextType() ==
               PredictionContextType::SINGLETON;
    }

    static bool is(const PredictionContext *predictionContext) {
        return predictionContext != nullptr && is(*predictionContext);
    }

    static Ref<const SingletonPredictionContext>
    create(Ref<const PredictionContext> parent, size_t returnState);

    const Ref<const PredictionContext> parent;
    const size_t returnState;

    SingletonPredictionContext(Ref<const PredictionContext> parent,
                               size_t returnState);

    bool isEmpty() const override;
    size_t size() const override;
    const Ref<const PredictionContext> &getParent(size_t index) const override;
    size_t getReturnState(size_t index) const override;
    bool equals(const PredictionContext &other) const override;
    std::string toString() const override;

  protected:
    size_t hashCodeImpl() const override;
};
} // namespace tagfilterdb::compiler::atn

#include "singletonPredictionContext.cpp"