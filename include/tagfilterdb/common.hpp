#pragma once

#include <algorithm>
#include <any>
#include <atomic>
#include <bitset>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <vector>

namespace tagfilterdb {
template <class T> using Ref = std::shared_ptr<T>;
template <class T> using Unique = std::unique_ptr<T>;
using Raw = void *;

} // namespace tagfilterdb
