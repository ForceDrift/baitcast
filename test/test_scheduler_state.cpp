#include "baitcast/detail/scheduler_state.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <stdexcept>
#include <thread>

namespace {
  using baitcast::detail::max_pending;
  using baitcast::detail::scheduler_state;

  struct probe {
    struct promise_type {
      probe get_return_object() { return probe{std::coroutine_handle<promise_type>::from_promise(*this)}; }
      std::suspend_always initial_suspend() noexcept { return {}; }
      std::suspend_always final_suspend() noexcept { return {}; }
      void return_void() noexcept {}
      void unhandled_exception() noexcept { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;

    explicit probe(std::coroutine_handle<promise_type> h) : handle(h) {}
    probe(const probe &) = delete;
    probe &operator=(const probe &) = delete;
    probe(probe &&) = delete;
    probe &operator=(probe &&) = delete;

    ~probe() {
      if (handle) {
        handle.destroy();
      }
    }
  };

  // Suspends at initial_suspend, so the body never runs unless the handle is resumed.
  probe make_probe(std::atomic<int> &resumed) {
    ++resumed;
    co_return;
  }
} // namespace

TEST(SchedulerState, EnqueueDequeuePreservesFifoOrder) {
  scheduler_state state;
  std::atomic<int> resumed{0};
  probe first = make_probe(resumed);
  probe second = make_probe(resumed);
  std::atomic<bool> stop{false};

  state.enqueue(first.handle);
  state.enqueue(second.handle);

  EXPECT_EQ(state.pending(), 2u);
  EXPECT_EQ(state.dequeue(stop), first.handle);
  EXPECT_EQ(state.pending(), 1u);
  EXPECT_EQ(state.dequeue(stop), second.handle);
  EXPECT_TRUE(state.empty());
}

TEST(SchedulerState, EnqueueBeyondCapacityThrowsLengthError) {
  scheduler_state state;
  std::coroutine_handle<> handle = std::noop_coroutine();
  for (std::size_t i = 0; i < max_pending; ++i) {
    state.enqueue(handle);
  }
  EXPECT_EQ(state.pending(), max_pending);
  EXPECT_THROW(state.enqueue(handle), std::length_error);
}

TEST(SchedulerState, EnqueueAfterShutdownThrowsLogicError) {
  scheduler_state state;
  state.shutdown();
  EXPECT_TRUE(state.is_shutdown());
  EXPECT_THROW(state.enqueue(std::noop_coroutine()), std::logic_error);
}

TEST(SchedulerState, DequeueBlocksUntilStopRequested) {
  scheduler_state state;
  std::atomic<bool> stop{false};
  std::coroutine_handle<> result = std::noop_coroutine();
  std::thread blocked{[&] { result = state.dequeue(stop); }};

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  EXPECT_EQ(result, std::noop_coroutine());

  stop.store(true, std::memory_order_release);
  state.notify();
  blocked.join();
  EXPECT_EQ(result, std::coroutine_handle<>{});
}

TEST(SchedulerState, ShutdownWakesBlockedDequeue) {
  scheduler_state state;
  std::atomic<bool> stop{false};
  std::coroutine_handle<> result = std::noop_coroutine();
  std::thread blocked{[&] { result = state.dequeue(stop); }};

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  state.shutdown();
  blocked.join();
  EXPECT_EQ(result, std::coroutine_handle<>{});
}