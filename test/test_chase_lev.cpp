#include "baitcast/detail/chase_lev.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>
#include <thread>

namespace {
  using baitcast::detail::chase_lev;
} // namespace

TEST(ChaseLev, PushPopBottomIsLifo) {
  chase_lev<int, 4> deque;
  EXPECT_TRUE(deque.push_bottom(1));
  EXPECT_TRUE(deque.push_bottom(2));
  EXPECT_TRUE(deque.push_bottom(3));

  EXPECT_EQ(deque.size(), 3u);
  ASSERT_EQ(deque.pop_bottom(), std::optional<int>{3});
  ASSERT_EQ(deque.pop_bottom(), std::optional<int>{2});
  ASSERT_EQ(deque.pop_bottom(), std::optional<int>{1});
  EXPECT_FALSE(deque.pop_bottom().has_value());
  EXPECT_TRUE(deque.empty());
}

TEST(ChaseLev, StealIsFifoFromTop) {
  chase_lev<int, 4> deque;
  EXPECT_TRUE(deque.push_bottom(1));
  EXPECT_TRUE(deque.push_bottom(2));
  EXPECT_TRUE(deque.push_bottom(3));

  ASSERT_EQ(deque.steal(), std::optional<int>{1});
  ASSERT_EQ(deque.steal(), std::optional<int>{2});
  ASSERT_EQ(deque.steal(), std::optional<int>{3});
  EXPECT_FALSE(deque.steal().has_value());
  EXPECT_TRUE(deque.empty());
}

TEST(ChaseLev, EmptyDequeYieldsNothing) {
  chase_lev<int, 4> deque;
  EXPECT_TRUE(deque.empty());
  EXPECT_FALSE(deque.steal().has_value());
  EXPECT_FALSE(deque.pop_bottom().has_value());
}

TEST(ChaseLev, PushBeyondCapacityReturnsFalse) {
  chase_lev<int, 2> deque;
  EXPECT_TRUE(deque.push_bottom(1));
  EXPECT_TRUE(deque.push_bottom(2));
  EXPECT_FALSE(deque.push_bottom(3));
  EXPECT_EQ(deque.size(), 2u);
}

TEST(ChaseLev, StealAndPopShareElements) {
  chase_lev<int, 4> deque;
  EXPECT_TRUE(deque.push_bottom(1));
  EXPECT_TRUE(deque.push_bottom(2));
  EXPECT_TRUE(deque.push_bottom(3));

  ASSERT_EQ(deque.steal(), std::optional<int>{1});
  ASSERT_EQ(deque.pop_bottom(), std::optional<int>{3});
  ASSERT_EQ(deque.steal(), std::optional<int>{2});
  EXPECT_TRUE(deque.empty());
}

TEST(ChaseLev, ConcurrentStealYieldsEachElementExactlyOnce) {
  constexpr std::size_t count = 1000;
  chase_lev<std::size_t, 2048> deque;
  std::array<std::atomic<int>, count> seen{};
  for (auto &slot : seen) {
    slot.store(0, std::memory_order_relaxed);
  }

  std::atomic<std::size_t> stolen{0};
  std::atomic<bool> start{false};
  auto thief = [&]() {
    while (!start.load(std::memory_order_acquire)) {
    }
    while (stolen.load(std::memory_order_acquire) < count) {
      if (std::optional<std::size_t> value = deque.steal()) {
        seen[*value].fetch_add(1, std::memory_order_relaxed);
        stolen.fetch_add(1, std::memory_order_relaxed);
      }
    }
  };

  std::thread first{thief};
  std::thread second{thief};
  start.store(true, std::memory_order_release);

  for (std::size_t i = 0; i < count; ++i) {
    while (!deque.push_bottom(i)) {
      std::this_thread::yield();
    }
  }

  first.join();
  second.join();

  for (std::size_t i = 0; i < count; ++i) {
    EXPECT_EQ(seen[i].load(std::memory_order_relaxed), 1) << "element " << i;
  }
  EXPECT_TRUE(deque.empty());
}
