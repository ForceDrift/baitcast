#pragma once
#include "align.hpp"
#include "inplace_vector.hpp"

#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <mutex>
#include <stdexcept>

namespace baitcast::detail {
  inline constexpr std::size_t max_pending = 256;

  static_assert(conc_handoff_safe<std::coroutine_handle<>>(), "baitcast::scheduler_state: queue elements must be trivially copyable for lock-protected handoff");
  static_assert(conc_alignment<std::coroutine_handle<>>() == alignof(std::coroutine_handle<>), "baitcast::scheduler_state: reflected alignment must match the compiler alignment");

  class scheduler_state {
    mutable std::mutex mutex_m;
    std::condition_variable cv_m;
    inplace_vector<std::coroutine_handle<>, max_pending> runnable_m;
    cache_aligned<std::atomic<bool>> shutdown_m{false};

  public:
    scheduler_state() = default;
    scheduler_state(const scheduler_state &) = delete;
    scheduler_state &operator=(const scheduler_state &) = delete;

    void enqueue(std::coroutine_handle<> handle) {
      std::lock_guard<std::mutex> lock{mutex_m};
      if (shutdown_m.value.load(std::memory_order_acquire)) {
        throw std::logic_error{"baitcast::detail::scheduler_state: enqueue after shutdown"};
      }
      if (runnable_m.full()) {
        throw std::length_error{"baitcast::detail::scheduler_state: queue capacity exceeded"};
      }
      runnable_m.push_back(handle);
      cv_m.notify_one();
    }

    void shutdown() noexcept {
      {
        std::lock_guard<std::mutex> lock{mutex_m};
        shutdown_m.value.store(true, std::memory_order_release);
      }
      cv_m.notify_all();
    }

    // Acquires mutex_m so a waiter cannot miss the wakeup between its predicate check
    // and registering on the condition variable.
    void notify() noexcept {
      std::lock_guard<std::mutex> lock{mutex_m};
      cv_m.notify_all();
    }

    [[nodiscard]] std::coroutine_handle<> dequeue(const std::atomic<bool> &stop) {
      std::unique_lock<std::mutex> lock{mutex_m};
      cv_m.wait(lock, [&]() { return stop.load(std::memory_order_acquire) || shutdown_m.value.load(std::memory_order_acquire) || !runnable_m.empty(); });
      if (runnable_m.empty()) {
        return std::coroutine_handle<>{};
      }
      std::coroutine_handle<> handle = runnable_m.front();
      runnable_m.erase(runnable_m.begin(), runnable_m.begin() + 1);
      return handle;
    }

    [[nodiscard]] bool is_shutdown() const noexcept { return shutdown_m.value.load(std::memory_order_acquire); }

    [[nodiscard]] bool empty() const noexcept {
      std::lock_guard<std::mutex> lock{mutex_m};
      return runnable_m.empty();
    }

    [[nodiscard]] std::size_t pending() const noexcept {
      std::lock_guard<std::mutex> lock{mutex_m};
      return runnable_m.size();
    }
  };
} // namespace baitcast::detail
