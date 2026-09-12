#include "baitcast/coroutine.hpp"
#include "baitcast/detail/task_registry.hpp"

#include <gtest/gtest.h>

namespace {
baitcast::detail::coroutine<int> make_task(int value) {
  co_return value;
}
} // namespace

TEST(TaskRegistry, StartsEmpty) {
  baitcast::detail::task_registry registry;
  EXPECT_TRUE(registry.empty());
  EXPECT_EQ(registry.size(), 0u);
}

TEST(TaskRegistry, InsertTracksChildren) {
  baitcast::detail::task_registry registry;
  registry.insert(make_task(1).release_handle());
  registry.insert(make_task(2).release_handle());
  registry.insert(make_task(3).release_handle());
  EXPECT_FALSE(registry.empty());
  EXPECT_EQ(registry.size(), 3u);
}

TEST(TaskRegistry, RemoveIfErasesCompletedOnly) {
  baitcast::detail::task_registry registry;
  auto a = make_task(1);
  auto b = make_task(2);
  auto ha = a.release_handle();
  auto hb = b.release_handle();
  registry.insert(ha);
  registry.insert(hb);

  ha.resume();

  auto removed = registry.remove_if([](baitcast::detail::task_slot &slot) { return slot.done(); });
  EXPECT_EQ(removed, 1u);
  EXPECT_EQ(registry.size(), 1u);

  hb.resume();
  removed = registry.remove_if([](baitcast::detail::task_slot &slot) { return slot.done(); });
  EXPECT_EQ(removed, 1u);
  EXPECT_TRUE(registry.empty());
}

TEST(TaskRegistry, ClearDropsAllChildren) {
  baitcast::detail::task_registry registry;
  registry.insert(make_task(1).release_handle());
  registry.insert(make_task(2).release_handle());
  registry.clear();
  EXPECT_TRUE(registry.empty());
}

TEST(TaskRegistry, IterationVisitsEveryChild) {
  baitcast::detail::task_registry registry;
  registry.insert(make_task(1).release_handle());
  registry.insert(make_task(2).release_handle());
  registry.insert(make_task(3).release_handle());

  std::size_t visited = 0;
  for (auto &&slot : registry) {
    EXPECT_FALSE(slot.done());
    ++visited;
  }
  EXPECT_EQ(visited, 3u);
}