#pragma once
#include "coroutine.hpp"
#include "detail/scheduler_state.hpp"
#include "detail/worker.hpp"

#include <coroutine>
#include <cstddef>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace baitcast {
  // A basic single-worker scheduler with rescheduling and clean shutdown.
  class scheduler {
    detail::scheduler_state state_m;
    detail::worker worker_m;
    std::mutex submit_mutex_m;
    bool shutdown_m = false;

  public:
    scheduler() : worker_m{state_m} {}
    explicit scheduler(detail::cpu_set affinity) : worker_m{state_m, std::move(affinity)} {}
    scheduler(const scheduler &) = delete;
    scheduler &operator=(const scheduler &) = delete;
    scheduler(scheduler &&) = delete;
    scheduler &operator=(scheduler &&) = delete;

    ~scheduler() { shutdown(); }

    struct yield_awaiter {
      detail::scheduler_state &state_m;

      [[nodiscard]] bool await_ready() const noexcept { return false; }
      void await_suspend(std::coroutine_handle<> handle) { state_m.enqueue(handle); }
      void await_resume() const noexcept {}
    };

    // co_await scheduler.yield() reschedules the current task onto the ready queue.
    [[nodiscard]] yield_awaiter yield() noexcept { return {state_m}; }

    template <typename T> void submit(detail::coroutine<T> &&task) {
      std::coroutine_handle<> handle = task.release_handle();
      std::lock_guard<std::mutex> lock{submit_mutex_m};
      if (shutdown_m) {
        if (handle) {
          handle.destroy();
        }
        throw std::logic_error{"baitcast::scheduler: submit after shutdown"};
      }
      try {
        state_m.enqueue(handle);
      } catch (...) {
        if (handle) {
          handle.destroy();
        }
        throw;
      }
    }

    void shutdown() noexcept {
      {
        std::lock_guard<std::mutex> lock{submit_mutex_m};
        shutdown_m = true;
      }
      worker_m.request_stop();
      worker_m.join();
    }

    [[nodiscard]] bool empty() const noexcept { return state_m.empty(); }

    [[nodiscard]] std::size_t pending() const noexcept { return state_m.pending(); }
  };
} // namespace baitcast