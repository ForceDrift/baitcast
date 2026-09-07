#pragma once
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace baitcast::detail {

#if defined(__cpp_lib_hardware_interference_size) && __cpp_lib_hardware_interference_size >= 201603L
  constexpr std::size_t cache_line_size = std::hardware_destructive_interference_size;
#else
  constexpr std::size_t cache_line_size = 64;
#endif

  /**
   * @struct algin
   * @breif serves as a wrapper so threads don't overlap on the same cache line
   * */

  template <typename T> struct alignas(cache_line_size) cache_aligned {
    T value;
    constexpr cache_aligned()
      requires std::is_default_constructible_v<T>
    = default;

    template <typename... Args> constexpr explicit cache_aligned(Args &&...args) : value(std::forward<Args>(args)...) {}

    constexpr T &operator*() noexcept { return value; }
    constexpr const T &operator*() const noexcept { return value; }

    constexpr T &operator->() noexcept { return std::addressof(value); }
    constexpr const T &operator->() const noexcept { return std::addressof(value); }

    constexpr T operator&() noexcept { return value; }
    constexpr const T operator&() const noexcept { return value; }
  };

}; // namespace baitcast::detail
