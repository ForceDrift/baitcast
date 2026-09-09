#include "baitcast/detail/align.hpp"

#include <gtest/gtest.h>

namespace {
static_assert(baitcast::detail::cache_line_size > 0, "Cache line size must be positive");
} // namespace

TEST(Align, CacheLineSizeIsPositive) {
  EXPECT_GT(baitcast::detail::cache_line_size, 0u);
}

TEST(Align, CacheAlignedValueDefaultConstructs) {
  baitcast::detail::cache_aligned<int> val{};
  EXPECT_EQ(*val, 0);
}