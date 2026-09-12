#include "baitcast/detail/worker.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <coroutine>
#include <thread>

namespace {
  using baitcast::detail::scheduler_state;
  using baitcast::detail::worker;

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

  // Suspends at initial_suspend; the body runs only once the worker resumes the handle.
  probe make_probe(std::atomic<int> &resumed) {
    ++resumed;
    co_return;
  }

  void wait_until(int target, const std::atomic<int> &value) {
    while (value.load() < target) {
      std::this_thread::yield();
    }
  }
} // namespace

TEST(Worker, ResumesEnqueuedCoroutines) {
  scheduler_state state;
  std::atomic<int> resumed{0};
  probe first = make_probe(resumed);
  probe second = make_probe(resumed);
  EXPECT_EQ(resumed.load(), 0);

  state.enqueue(first.handle);
  state.enqueue(second.handle);

  {
    worker w{state};
    wait_until(2, resumed);
  }

  EXPECT_EQ(resumed.load(), 2);
  EXPECT_TRUE(state.empty());
}

TEST(Worker, ResumesInEnqueueOrder) {
  scheduler_state state;
  std::atomic<int> next{0};
  std::array<int, 4> order{};
  auto make = [&](int id) -> probe {
    order[next.fetch_add(1)] = id;
    co_return;
  };

  {
    worker w{state};
    probe first = make(1);
    probe second = make(2);
    probe third = make(3);
    probe fourth = make(4);
    state.enqueue(first.handle);
    state.enqueue(second.handle);
    state.enqueue(third.handle);
    state.enqueue(fourth.handle);
    wait_until(4, next);
  }

  EXPECT_EQ(order[0], 1);
  EXPECT_EQ(order[1], 2);
  EXPECT_EQ(order[2], 3);
  EXPECT_EQ(order[3], 4);
  EXPECT_TRUE(state.empty());
}

TEST(Worker, DrainsPendingWorkBeforeExiting) {
  scheduler_state state;
  std::atomic<int> resumed{0};
  probe first = make_probe(resumed);

  worker w{state};
  state.enqueue(first.handle);
  w.request_stop();
  w.join();

  EXPECT_EQ(resumed.load(), 1);
  EXPECT_TRUE(state.empty());
}

TEST(Worker, DestructionStopsIdleWorker) {
  scheduler_state state;
  {
    worker w{state};
  }
  EXPECT_TRUE(state.empty());
}