#include "baitcast/detail/inplace_vector.hpp"

#include <gtest/gtest.h>

#include <iterator>
#include <new>
#include <stdexcept>

namespace {
  using baitcast::detail::inplace_vector;

  struct counting {
    static inline int alive = 0;
    counting() { ++alive; }
    counting(const counting &) { ++alive; }
    counting(counting &&) noexcept { ++alive; }
    counting &operator=(const counting &) = default;
    counting &operator=(counting &&) noexcept = default;
    ~counting() { --alive; }
  };
} // namespace

TEST(InplaceVector, GrowsToCapacityWithoutAllocation) {
  inplace_vector<int, 4> v;
  EXPECT_TRUE(v.empty());
  EXPECT_EQ(v.capacity(), 4u);
  EXPECT_EQ(v.max_size(), 4u);
  v.push_back(1);
  v.push_back(2);
  v.push_back(3);
  v.push_back(4);
  EXPECT_FALSE(v.empty());
  EXPECT_TRUE(v.full());
  EXPECT_EQ(v.size(), 4u);
  EXPECT_EQ(v[0], 1);
  EXPECT_EQ(v.back(), 4);
}

TEST(InplaceVector, FullPushThrowsBadAlloc) {
  inplace_vector<int, 2> v;
  v.push_back(1);
  v.push_back(2);
  EXPECT_THROW(v.push_back(3), std::bad_alloc);
}

TEST(InplaceVector, EraseRemovesRange) {
  inplace_vector<int, 8> v;
  v.push_back(1);
  v.push_back(2);
  v.push_back(3);
  v.push_back(4);
  v.push_back(5);

  auto it = v.erase(v.begin() + 1, v.begin() + 3);
  EXPECT_EQ(*it, 4);
  EXPECT_EQ(v.size(), 3u);
  EXPECT_EQ(v[0], 1);
  EXPECT_EQ(v[1], 4);
  EXPECT_EQ(v[2], 5);
}

TEST(InplaceVector, PopBackDestroysAndShrinks) {
  inplace_vector<counting, 4> v;
  v.emplace_back();
  v.emplace_back();
  EXPECT_EQ(counting::alive, 2);
  v.pop_back();
  EXPECT_EQ(counting::alive, 1);
  EXPECT_EQ(v.size(), 1u);
}

TEST(InplaceVector, LifecycleDestroysLiveElements) {
  {
    inplace_vector<counting, 4> v;
    v.emplace_back();
    v.emplace_back();
    v.emplace_back();
    EXPECT_EQ(counting::alive, 3);
  }
  EXPECT_EQ(counting::alive, 0);
}

TEST(InplaceVector, IterationVisitsEveryElement) {
  inplace_vector<int, 4> v;
  v.push_back(0);
  v.push_back(1);
  v.push_back(2);
  int sum = 0;
  for (int value : v) {
    sum += value;
  }
  EXPECT_EQ(sum, 3);
  EXPECT_EQ(std::distance(v.begin(), v.end()), 3);
}

TEST(InplaceVector, AtBoundsChecks) {
  inplace_vector<int, 2> v;
  v.push_back(7);
  EXPECT_EQ(v.at(0), 7);
  EXPECT_THROW(static_cast<void>(v.at(1)), std::out_of_range);
}