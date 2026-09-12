#include "baitcast/nursery.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <stdexcept>

namespace {
  int completed_count = 0;

  baitcast::detail::coroutine<int> counter_task() {
    co_await std::suspend_always{};
    ++completed_count;
    co_return completed_count;
  }

  baitcast::detail::coroutine<int> nested_spawner() {
    baitcast::nursery inner;
    inner.spawn(counter_task());
    inner.spawn(counter_task());
    co_return 7;
  }

  void create_and_abandon_task() {
    baitcast::detail::coroutine<int> task = counter_task();
    (void)task;
  }
} // namespace

TEST(Nursery, WaitCompletesSpawnedChildren) {
  completed_count = 0;
  {
    baitcast::nursery n;
    n.spawn(counter_task());
    n.spawn(counter_task());
    n.spawn(counter_task());
    n.wait();
  }
  EXPECT_EQ(completed_count, 3);
}

TEST(Nursery, WaitDrivesChildrenThroughIntermediateSuspension) {
  completed_count = 0;
  {
    baitcast::nursery n;
    n.spawn(counter_task());
    n.spawn(counter_task());
    n.wait();
  }
  EXPECT_EQ(completed_count, 2);
}

TEST(Nursery, DestructorWaitsForChildren) {
  completed_count = 0;
  {
    baitcast::nursery n;
    n.spawn(counter_task());
    n.spawn(counter_task());
  }
  EXPECT_EQ(completed_count, 2);
}

TEST(Nursery, NestedNurserieCompletesChildrenBeforeContinuing) {
  completed_count = 0;
  {
    baitcast::nursery n;
    n.spawn(nested_spawner());
    n.wait();
  }
  EXPECT_EQ(completed_count, 2);
}

TEST(Nursery, ScopeCompletionIsTracked) {
  baitcast::nursery n;
  EXPECT_FALSE(n.is_complete());
  n.spawn(counter_task());
  n.wait();
  EXPECT_TRUE(n.is_complete());
}

TEST(Nursery, WaitOnEmptyNurseryCompletesImmediately) {
  baitcast::nursery n;
  n.wait();
  EXPECT_TRUE(n.is_complete());
}

TEST(Nursery, SpawnAfterCompleteThrows) {
  completed_count = 0;
  baitcast::nursery n;
  n.spawn(counter_task());
  n.wait();
  EXPECT_THROW(n.spawn(counter_task()), std::logic_error);
}

TEST(Nursery, SpawnBeyondCapacityThrows) {
  completed_count = 0;
  {
    baitcast::nursery n;
    for (std::size_t i = 0; i < baitcast::detail::max_children; ++i) {
      n.spawn(counter_task());
    }
    EXPECT_THROW(n.spawn(counter_task()), std::length_error);
  }
  EXPECT_EQ(completed_count, static_cast<int>(baitcast::detail::max_children));
}

TEST(NurseryDeathTest, DestroyingDetachedTaskTerminates) { EXPECT_DEATH(create_and_abandon_task(), ".*"); }