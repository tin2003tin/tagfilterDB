#pragma once

#include "tagfilterdb/common.hpp"

namespace tagfilterdb::support {
template <typename T1, typename T2> inline bool is(T2 *obj) {
    return dynamic_cast<typename std::add_const<T1>::type>(obj) != nullptr;
}

template <typename T1, typename T2> inline bool is(Ref<T2> const &obj) {
    return dynamic_cast<T1 *>(obj.get()) != nullptr;
}

std::map<std::string, size_t> toMap(const std::vector<std::string> &keys) {
    std::map<std::string, size_t> result;
    for (size_t i = 0; i < keys.size(); ++i) {
        result.insert({keys[i], i});
    }
    return result;
}

template <typename OnEnd> struct FinalAction {
    FinalAction(OnEnd f) : _cleanUp{std::move(f)} {}
    FinalAction(FinalAction &&other)
        : _cleanUp(std::move(other._cleanUp)), _enabled(other._enabled) {
        other._enabled =
            false; // Don't trigger the lambda after ownership has moved.
    }
    ~FinalAction() {
        if (_enabled)
            _cleanUp();
    }

    void disable() { _enabled = false; }

  private:
    OnEnd _cleanUp;
    bool _enabled{true};
};

template <typename OnEnd> FinalAction<OnEnd> finally(OnEnd f) {
    return FinalAction<OnEnd>(std::move(f));
}

} // namespace tagfilterdb::support