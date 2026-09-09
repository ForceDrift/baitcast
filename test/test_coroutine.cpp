#include "baitcast/coroutine.hpp"

#include <gtest/gtest.h>

baitcast::detail::coroutine<int> produce_value() {
  co_return 42;
}

baitcast::detail::coroutine<int> consume_value() {
  int val = co_await produce_value();
  co_return val * 2;
}

TEST(Coroutine, ChainedCoAwaitCoReturn) {
  auto task = consume_value();
  task.handle().resume();
  auto result = task.result();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 84);
}