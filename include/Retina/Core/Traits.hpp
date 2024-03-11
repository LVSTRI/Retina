#pragma once

#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Types.hpp>

#include <type_traits>

namespace Retina::Core {
  template <template <typename...> typename, typename>
  struct SIsSpecialization : std::false_type {};

  template <template <typename...> typename T, typename... Ts>
  struct SIsSpecialization<T, T<Ts...>> : std::true_type {};

  template <template <typename...> typename T, typename U>
  consteval auto IsSpecialization() noexcept -> bool {
    return SIsSpecialization<T, U>::value;
  }

  template <typename T, typename... Ts>
  concept TContains = std::disjunction_v<std::is_same<T, Ts>...>;
}
