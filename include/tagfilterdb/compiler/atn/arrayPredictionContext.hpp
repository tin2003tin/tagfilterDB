#pragma once

#include "predictionContext.hpp"

namespace tagfilterdb::compiler::atn {

class ArrayPredictionContext final : public PredictionContext {
  public:
    static bool is(const PredictionContext &predictionContext) {
        return predictionContext.getContextType() ==
               PredictionContextType::ARRAY;
    }

    static bool is(const PredictionContext *predictionContext) {
        return predictionContext != nullptr && is(*predictionContext);
    }

    std::vector<Ref<const PredictionContext>> parents;

    std::vector<size_t> returnStates;

    explicit ArrayPredictionContext(
        const SingletonPredictionContext &predictionContext);

    ArrayPredictionContext(std::vector<Ref<const PredictionContext>> parents,
                           std::vector<size_t> returnStates);

    ArrayPredictionContext(ArrayPredictionContext &&) = default;

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

#include "arrayPredictionContext.cpp"