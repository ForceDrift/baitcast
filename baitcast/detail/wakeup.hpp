#pragma once
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <utility>

namespace baitcast::detail {
  // A parking/notification primitive used to avoid busy-waiting when a worker has no
  // runnable work. The waiter parks on a predicate that is re-evaluated while holding
  // the queue's mutex, and producers publish under that same mutex before notifying, so
  // no wakeup can be lost between a final predicate check and actually parking.
  class wakeup {
    std::condition_variable cv_m;

  public:
    wakeup() = default;
    wakeup(const wakeup &) = delete;
    wakeup &operator=(const wakeup &) = delete;

    // Blocks until pred() is true. pred is re-checked after spurious wakeups and must be
    // read/written under lock.
    template <typename Predicate> void wait(std::unique_lock<std::mutex> &lock, Predicate &&predicate) {
      cv_m.wait(lock, std::forward<Predicate>(predicate));
    }

    // Blocks until pred() is true or the relative timeout elapses. Returns the predicate result.
    template <typename Rep, typename Period, typename Predicate>
    bool wait_for(std::unique_lock<std::mutex> &lock, const std::chrono::duration<Rep, Period> &timeout, Predicate &&predicate) {
      return cv_m.wait_for(lock, timeout, std::forward<Predicate>(predicate));
    }

    void notify_one() noexcept { cv_m.notify_one(); }
    void notify_all() noexcept { cv_m.notify_all(); }
  };
} // namespace baitcast::detail
