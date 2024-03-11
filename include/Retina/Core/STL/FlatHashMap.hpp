#pragma once

#include <Retina/Core/STL/Hash.hpp>

namespace Retina::Core {
  template <typename K, typename V>
  using FlatHashMap = unordered_dense::map<K, V>;
}