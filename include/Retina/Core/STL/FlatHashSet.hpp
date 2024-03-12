#pragma once

#include <Retina/Core/STL/Hash.hpp>

namespace Retina::Core {
  template <typename K>
  using FlatHashSet = unordered_dense::set<K>;
}