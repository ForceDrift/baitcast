#pragma once
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#if defined(__cpp_impl_reflection) && __cpp_impl_reflection >= 202603L
#include <meta>
#endif

namespace baitcast::detail {

#if defined(__cpp_impl_reflection) && __cpp_impl_reflection >= 202603L
  // Whether T can be handed between threads under a lock without per-element lifetime hooks.
  template <typename T> consteval bool conc_handoff_safe() noexcept { return std::meta::is_object_type(^^T) && std::meta::is_trivially_copyable_type(^^T); }

  template <typename T> consteval std::size_t conc_alignment() noexcept { return std::meta::alignment_of(^^T); }

  // Aggregate alignment derived from T's reflected non-static data members.
  template <typename T> consteval std::size_t conc_layout_alignment() noexcept {
    std::size_t alignment = 1;
    for (auto member : std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked())) {
      std::size_t member_alignment = std::meta::alignment_of(std::meta::type_of(member));
      if (member_alignment > alignment) {
        alignment = member_alignment;
      }
    }
    return alignment;
  }
#else
  template <typename T> constexpr bool conc_handoff_safe() noexcept { return std::is_object_v<T> && std::is_trivially_copyable_v<T>; }

  template <typename T> constexpr std::size_t conc_alignment() noexcept { return alignof(T); }

  template <typename T> constexpr std::size_t conc_layout_alignment() noexcept { return alignof(T); }
#endif

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
    static_assert(std::is_object_v<T>, "cache_aligned<T> requires an object type");
    T value;
    constexpr cache_aligned()
      requires std::is_default_constructible_v<T>
    = default;

    template <typename... Args>
      requires std::is_constructible_v<T, Args &&...>
    constexpr explicit cache_aligned(Args &&...args) : value(std::forward<Args>(args)...) {}

    constexpr T &operator*() noexcept { return value; }
    constexpr const T &operator*() const noexcept { return value; }

    constexpr T &operator->() noexcept { return std::addressof(value); }
    constexpr const T &operator->() const noexcept { return std::addressof(value); }

    [[nodiscard]] constexpr T operator&() noexcept
      requires std::is_copy_constructible_v<T>
    {
      return value;
    }
    [[nodiscard]] constexpr const T operator&() const noexcept
      requires std::is_copy_constructible_v<T>
    {
      return value;
    }
  };

}; // namespace baitcast::detail
