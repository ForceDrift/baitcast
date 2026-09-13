#include "baitcast/scheduler.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>

namespace {
  using baitcast::scheduler;
  using baitcast::detail::coroutine;

  coroutine<int> incrementing(std::atomic<int> &count) {
    ++count;
    co_return 1;
  }

  coroutine<int> repeating(scheduler &s, std::atomic<int> &count, int iterations) {
    for (int i = 0; i < iterations; ++i) {
      ++count;
      co_await s.yield();
    }
    co_return iterations;
  }

  void wait_until(const std::atomic<int> &value, int target) {
    while (value.load() < target) {
      std::this_thread::yield();
    }
  }
} // namespace

TEST(Scheduler, SubmitsAndCompletesOnWorker) {
  scheduler s;
  std::atomic<int> count{0};
  s.submit(incrementing(count));
  wait_until(count, 1);
  s.shutdown();
  EXPECT_EQ(count.load(), 1);
  EXPECT_TRUE(s.empty());
}

TEST(Scheduler, ReschedulesViaYield) {
  scheduler s;
  std::atomic<int> count{0};
  s.submit(repeating(s, count, 5));
  wait_until(count, 5);
  s.shutdown();
  EXPECT_EQ(count.load(), 5);
  EXPECT_TRUE(s.empty());
}

TEST(Scheduler, ShutdownDrainsPendingWork) {
  scheduler s;
  std::atomic<int> count{0};
  s.submit(incrementing(count));
  s.submit(incrementing(count));
  s.shutdown();
  EXPECT_EQ(count.load(), 2);
  EXPECT_TRUE(s.empty());
}

TEST(Scheduler, ShutdownDrainsReschedulingTasks) {
  scheduler s;
  std::atomic<int> count{0};
  s.submit(repeating(s, count, 3));
  s.submit(repeating(s, count, 3));
  s.shutdown();
  EXPECT_EQ(count.load(), 6);
  EXPECT_TRUE(s.empty());
}

TEST(Scheduler, SubmitAfterShutdownThrowsLogicError) {
  scheduler s;
  s.shutdown();
  std::atomic<int> count{0};
  EXPECT_THROW(s.submit(incrementing(count)), std::logic_error);
}

TEST(Scheduler, DestructorShutsDownCleanly) {
  std::atomic<int> count{0};
  {
    scheduler s;
    s.submit(incrementing(count));
    wait_until(count, 1);
  }
  EXPECT_EQ(count.load(), 1);
}