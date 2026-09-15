#pragma once
#include "align.hpp"
#include "inplace_vector.hpp"

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <mutex>
#include <stdexcept>

namespace baitcast::detail {
  inline constexpr std::size_t max_pending = 256;

  template <std::size_t Capacity = max_pending> class ingress_queue {
    mutable std::mutex mutex_m;
    cache_aligned<std::atomic<bool>> shutdown_m{false};
    inplace_vector<std::coroutine_handle<>, Capacity> runnable_m;

  public:
    ingress_queue() = default;
    ingress_queue(const ingress_queue &) = delete;
    ingress_queue &operator=(const ingress_queue &) = delete;

    [[nodiscard]] std::mutex &mutex() const noexcept { return mutex_m; }

    void push_locked(std::coroutine_handle<> handle) {
      if (shutdown_m.value.load(std::memory_order_acquire)) {
        throw std::logic_error{"baitcast::detail::ingress_queue: push after shutdown"};
      }
      if (runnable_m.full()) {
        throw std::length_error{"baitcast::detail::ingress_queue: queue capacity exceeded"};
      }
      runnable_m.push_back(handle);
    }

    // Caller must hold mutex(). Returns std::nullopt when empty.
    [[nodiscard]] std::coroutine_handle<> pop_locked() {
      if (runnable_m.empty()) {
        return std::coroutine_handle<>{};
      }
      std::coroutine_handle<> handle = runnable_m.front();
      runnable_m.erase(runnable_m.begin(), runnable_m.begin() + 1);
      return handle;
    }

    // Convenience wrappers that lock internally.
    void push(std::coroutine_handle<> handle) {
      std::lock_guard<std::mutex> lock{mutex_m};
      push_locked(handle);
    }
    [[nodiscard]] std::coroutine_handle<> pop() {
      std::lock_guard<std::mutex> lock{mutex_m};
      return pop_locked();
    }

    void set_shutdown_locked() noexcept { shutdown_m.value.store(true, std::memory_order_release); }

    void request_shutdown() noexcept {
      {
        std::lock_guard<std::mutex> lock{mutex_m};
        set_shutdown_locked();
      }
    }

    [[nodiscard]] bool is_shutdown() const noexcept { return shutdown_m.value.load(std::memory_order_acquire); }

    [[nodiscard]] bool empty_locked() const noexcept { return runnable_m.empty(); }
    [[nodiscard]] std::size_t pending_locked() const noexcept { return runnable_m.size(); }

    [[nodiscard]] bool empty() const noexcept {
      std::lock_guard<std::mutex> lock{mutex_m};
      return empty_locked();
    }
    [[nodiscard]] std::size_t pending() const noexcept {
      std::lock_guard<std::mutex> lock{mutex_m};
      return pending_locked();
    }
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }
  };
} // namespace baitcast::detail
