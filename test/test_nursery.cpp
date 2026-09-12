#include "baitcast/nursery.hpp"

#include <gtest/gtest.h>

namespace {
int completed_count = 0;

baitcast::detail::coroutine<int> counter_task() {
  co_await std::suspend_always{};
  ++completed_count;
  co_return completed_count;
}

baitcast::detail::coroutine<int> nested_spawner(baitcast::nursery *n) {
  n->spawn(counter_task());
  n->spawn(counter_task());
  co_return 7;
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

TEST(Nursery, ChildrenCanSpawnGrandchildren) {
  completed_count = 0;
  {
    baitcast::nursery n;
    n.spawn(nested_spawner(&n));
    n.wait();
  }
  EXPECT_EQ(completed_count, 2);
}

TEST(Nursery, WaitOnEmptyNurseryReturnsImmediately) {
  baitcast::nursery n;
  n.wait();
}

TEST(Nursery, DestructorClearsPendingChildren) {
  completed_count = 0;
  {
    baitcast::nursery n;
    n.spawn(counter_task());
    n.spawn(counter_task());
  }
}