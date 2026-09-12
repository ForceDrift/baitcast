#include "baitcast/coroutine.hpp"

#include <gtest/gtest.h>

baitcast::detail::coroutine<int> produce_value() { co_return 42; }

baitcast::detail::coroutine<int> consume_value() {
  int val = co_await produce_value();
  co_return val * 2;
}

static_assert(!std::is_copy_constructible_v<baitcast::detail::coroutine<int>>);
static_assert(std::is_move_constructible_v<baitcast::detail::coroutine<int>>);
static_assert(!std::is_copy_assignable_v<baitcast::detail::coroutine<int>>);
static_assert(std::is_move_assignable_v<baitcast::detail::coroutine<int>>);

TEST(Coroutine, ChainedCoAwaitCoReturn) {
  auto task = consume_value();
  EXPECT_EQ(*task.state(), baitcast::detail::task_status::SUSPENDED);
  task.handle().resume();
  EXPECT_EQ(*task.state(), baitcast::detail::task_status::COMPLETED);
  auto result = task.result();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 84);
}