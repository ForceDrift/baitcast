#include "baitcast/detail/align.hpp"

#include <gtest/gtest.h>

namespace {
  static_assert(baitcast::detail::cache_line_size > 0, "Cache line size must be positive");

  struct no_default {
    explicit no_default(int) {}
  };

  struct no_copy {
    no_copy() = default;
    no_copy(const no_copy &) = delete;
    no_copy &operator=(const no_copy &) = delete;
  };

  static_assert(std::is_object_v<baitcast::detail::cache_aligned<int>>);
  static_assert(std::is_copy_constructible_v<baitcast::detail::cache_aligned<int>>);
  static_assert(std::is_default_constructible_v<baitcast::detail::cache_aligned<int>>);
  static_assert(!std::is_default_constructible_v<baitcast::detail::cache_aligned<no_default>>);
  static_assert(std::is_constructible_v<baitcast::detail::cache_aligned<no_default>, int>);

  template <typename T> constexpr bool has_value_operator() {
    return requires(T value) { static_cast<decltype(value.value)>(value.operator&()); };
  }
  static_assert(has_value_operator<baitcast::detail::cache_aligned<int>>());
  static_assert(!has_value_operator<baitcast::detail::cache_aligned<no_copy>>());
} // namespace

TEST(Align, CacheLineSizeIsPositive) { EXPECT_GT(baitcast::detail::cache_line_size, 0u); }

TEST(Align, CacheAlignedValueDefaultConstructs) {
  baitcast::detail::cache_aligned<int> val{};
  EXPECT_EQ(*val, 0);
}