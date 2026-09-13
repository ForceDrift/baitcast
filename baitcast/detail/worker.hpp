#pragma once
#include "align.hpp"
#include "scheduler_state.hpp"
#include "topology.hpp"

#include <atomic>
#include <coroutine>
#include <thread>
#include <utility>

namespace baitcast::detail {
  class worker {
    scheduler_state &state_m;
    cpu_set affinity_m;
    cache_aligned<std::atomic<bool>> stop_m{false};
    std::thread thread_m;

    void run() {
      (void)pin_current_thread(affinity_m);
      for (std::coroutine_handle<> handle = state_m.dequeue(stop_m.value); handle; handle = state_m.dequeue(stop_m.value)) {
        handle.resume();
        if (handle.done()) {
          handle.destroy();
        }
      }
    }

  public:
    explicit worker(scheduler_state &state, cpu_set affinity = {}) : state_m(state), affinity_m(std::move(affinity)), thread_m(&worker::run, this) {}

    worker(const worker &) = delete;
    worker &operator=(const worker &) = delete;
    worker(worker &&) = delete;
    worker &operator=(worker &&) = delete;

    void request_stop() noexcept {
      stop_m.value.store(true, std::memory_order_release);
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