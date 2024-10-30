#pragma once

#include "predictionContext.hpp"

namespace tagfilterdb::compiler::atn {
template <typename Key, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>,
          typename Allocator = std::allocator<Key>>
using FlatHashSet = std::unordered_set<Key, Hash, Equal, Allocator>;

class PredictionContextCache final {
  public:
    PredictionContextCache() = default;

    PredictionContextCache(const PredictionContextCache &) = delete;
    PredictionContextCache(PredictionContextCache &&) = delete;

    PredictionContextCache &operator=(const PredictionContextCache &) = delete;
    PredictionContextCache &operator=(PredictionContextCache &&) = delete;

    void put(const Ref<const PredictionContext> &value);

    Ref<const PredictionContext>
    get(const Ref<const PredictionContext> &value) const;

  private:
    struct ANTLR4CPP_PUBLIC PredictionContextHasher final {
        size_t
        operator()(const Ref<const PredictionContext> &predictionContext) const;
    };

    struct ANTLR4CPP_PUBLIC PredictionContextComparer final {
        bool operator()(const Ref<const PredictionContext> &lhs,
                        const Ref<const PredictionContext> &rhs) const;
    };

    FlatHashSet<Ref<const PredictionContext>, PredictionContextHasher,
                PredictionContextComparer>
        _data;
};
} // namespace tagfilterdb::compiler::atn