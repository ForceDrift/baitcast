#pragma once
#include "scheduler_state.hpp"

#include <atomic>
#include <coroutine>
#include <thread>

namespace baitcast::detail {
  class worker {
    scheduler_state &state_m;
    std::atomic<bool> stop_m{false};
    std::thread thread_m;

    void run() {
      for (std::coroutine_handle<> handle = state_m.dequeue(stop_m); handle; handle = state_m.dequeue(stop_m)) {
        handle.resume();
      }
    }

  public:
    explicit worker(scheduler_state &state) : state_m(state), thread_m(&worker::run, this) {}

    worker(const worker &) = delete;
    worker &operator=(const worker &) = delete;
    worker(worker &&) = delete;
    worker &operator=(worker &&) = delete;

    void request_stop() noexcept {
      stop_m.store(true, std::memory_order_release);
      state_m.notify();
    }

    void join() {
      if (thread_m.joinable()) {
        thread_m.join();
      }
    }

    ~worker() {
      request_stop();
      join();
    }
  };
} // namespace baitcast::detail