#pragma once

#include "tagfilterdb/common.hpp"

namespace tagfilterdb::support {
template <typename T1, typename T2> inline bool is(T2 *obj) {
    return dynamic_cast<typename std::add_const<T1>::type>(obj) != nullptr;
}

template <typename T1, typename T2> inline bool is(Ref<T2> const &obj) {
    return dynamic_cast<T1 *>(obj.get()) != nullptr;
}
} // namespace tagfilterdb::support