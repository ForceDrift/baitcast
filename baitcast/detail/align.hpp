#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>
namespace baitcast::detail {
#if defined(__cpp_lib_hardware_interference_size) &&                           \
    __cpp_lib_hardware_interference_size >= 201603L
  // Standard C++ constants
  constexpr std::size_t cache_line_size =
      std::hardware_destructive_interference_size;
#else
// Fallback alignment: 64 bytes for standard x86_64/ARM64, 128 bytes for Apple
// Silicon/IBM POWER
#if defined(__aarch64__) && defined(__APPLE__)
  constexpr std::size_t cache_line_size = 128;
#else
  constexpr std::size_t cache_line_size = 64;
#endif
#endif

  template <typename T> struct alignas(cache_line_size) cache_aligned {
    T value;
    constexpr cache_aligned() std::is_default_constructible_v<T> = default;
    constexpr explicit cache_aligned(T val) : value(std::move(val)) {}

    constexpr T &get() noexcept { return value; }
    constexpr const T &get() const noexcept { return value; }

    constexpr operator T &() noexcept { return value; }
    constexpr operator const T &() const noexcept { return value; }
  };

}; // namespace baitcast::detail
