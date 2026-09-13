#pragma once
#include "inplace_vector.hpp"

#include <cstddef>
#include <initializer_list>
#include <optional>
#include <thread>
#include <utility>

#if defined(__linux__)
#include <pthread.h>
#include <sched.h>
#elif defined(__APPLE__)
#include <mach/kern_return.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace baitcast::detail {
  using cpu_id = std::size_t;

  inline constexpr std::size_t max_cpus = 256;

  // A capacity-bounded set of CPU ids. Never allocates; duplicates are ignored.
  class cpu_set {
  public:
    using value_type = cpu_id;
    using size_type = std::size_t;
    using const_iterator = inplace_vector<cpu_id, max_cpus>::const_iterator;

    cpu_set() = default;
    explicit cpu_set(size_type count) { add_range(0, count); }

    cpu_set(std::initializer_list<cpu_id> cpus) {
      for (cpu_id cpu : cpus) {
        add(cpu);
      }
    }

    [[nodiscard]] constexpr size_type size() const noexcept { return cpus_m.size(); }
    [[nodiscard]] constexpr bool empty() const noexcept { return cpus_m.empty(); }
    [[nodiscard]] constexpr bool full() const noexcept { return cpus_m.full(); }

    [[nodiscard]] bool add(cpu_id cpu) noexcept {
      if (contains(cpu)) {
        return true;
      }
      if (cpus_m.full()) {
        return false;
      }
      cpus_m.push_back(cpu);
      return true;
    }

    [[nodiscard]] bool remove(cpu_id cpu) noexcept {
      const_iterator it = find(cpu);
      if (it == cend()) {
        return false;
      }
      cpus_m.erase(it, it + 1);
      return true;
    }

    [[nodiscard]] constexpr bool contains(cpu_id cpu) const noexcept { return find(cpu) != cend(); }

    [[nodiscard]] constexpr cpu_id operator[](size_type i) const noexcept { return cpus_m[i]; }

    [[nodiscard]] constexpr const_iterator begin() const noexcept { return cpus_m.begin(); }
    [[nodiscard]] constexpr const_iterator end() const noexcept { return cpus_m.end(); }
    [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return cpus_m.cbegin(); }
    [[nodiscard]] constexpr const_iterator cend() const noexcept { return cpus_m.cend(); }

  private:
    void add_range(cpu_id first, size_type count) noexcept {
      for (cpu_id i = 0; i < count && !cpus_m.full(); ++i) {
        cpus_m.push_back(first + i);
      }
    }

    [[nodiscard]] constexpr const_iterator find(cpu_id cpu) const noexcept {
      for (const_iterator it = cbegin(); it != cend(); ++it) {
        if (*it == cpu) {
          return it;
        }
      }
      return cend();
    }

    inplace_vector<cpu_id, max_cpus> cpus_m;
  };

  // Number of CPUs the process may bind to.
  [[nodiscard]] cpu_id cpu_count() noexcept {
#if defined(__linux__)
    cpu_set_t set{};
    if (::sched_getaffinity(0, sizeof(set), &set) == 0) {
      cpu_id count = 0;
      for (cpu_id i = 0; i < CPU_SETSIZE; ++i) {
        if (CPU_ISSET(i, &set)) {
          ++count;
        }
      }
      if (count > 0) {
        return count;
      }
    }
#endif
    unsigned int detected = std::thread::hardware_concurrency();
    return detected == 0 ? 1 : static_cast<cpu_id>(detected);
  }

  // The set of CPUs the process may bind to.
  [[nodiscard]] cpu_set online_cpus() noexcept {
#if defined(__linux__)
    cpu_set_t set{};
    if (::sched_getaffinity(0, sizeof(set), &set) == 0) {
      cpu_set result;
      for (cpu_id i = 0; i < CPU_SETSIZE && !result.full(); ++i) {
        if (CPU_ISSET(i, &set)) {
          result.add(i);
        }
      }
      if (!result.empty()) {
        return result;
      }
    }
#endif
    return cpu_set(cpu_count());
  }

  // Maps a worker index onto the CPU set, cycling through it.
  [[nodiscard]] constexpr std::optional<cpu_id> cpu_for(cpu_id worker_index, const cpu_set &set) noexcept {
    if (set.empty()) {
      return std::nullopt;
    }
    return set[worker_index % set.size()];
  }

  // Applies the CPU set to the calling thread; no-op for an empty set. Best effort on
  // platforms without hard affinity (e.g. macOS uses an advisory affinity tag).
  [[nodiscard]] bool pin_current_thread(const cpu_set &cpus) noexcept {
    if (cpus.empty()) {
      return true;
    }
#if defined(__linux__)
    cpu_set_t set{};
    CPU_ZERO(&set);
    for (cpu_id cpu : cpus) {
      if (cpu >= CPU_SETSIZE) {
        return false;
      }
      CPU_SET(cpu, &set);
    }
    return ::pthread_setaffinity_np(::pthread_self(), sizeof(set), &set) == 0;
#elif defined(__APPLE__)
    thread_affinity_policy_data_t policy{static_cast<integer_t>(cpus[0])};
    return ::thread_policy_set(::pthread_mach_thread_np(::pthread_self()), THREAD_AFFINITY_POLICY,
                               reinterpret_cast<thread_policy_t>(&policy), THREAD_AFFINITY_POLICY_COUNT) == KERN_SUCCESS;
#elif defined(_WIN32)
    DWORD_PTR mask = 0;
    for (cpu_id cpu : cpus) {
      if (cpu >= sizeof(DWORD_PTR) * CHAR_BIT) {
        return false;
      }
      mask |= DWORD_PTR{1} << cpu;
    }
    return ::SetThreadAffinityMask(::GetCurrentThread(), mask) != 0;
#else
    (void)cpus;
    return false;
#endif
  }

  // Process topology: detected CPUs plus a configurable allowed set. NUMA-aware
  // organization and locality-aware stealing are deferred optimizations.
  class topology {
  public:
    explicit topology(cpu_set allowed = {}) : allowed_m(std::move(allowed)) {}

    [[nodiscard]] cpu_id cpu_count() const noexcept { return baitcast::detail::cpu_count(); }
    [[nodiscard]] const cpu_set &allowed_cpus() const noexcept { return allowed_m; }

    void set_allowed_cpus(cpu_set allowed) noexcept { allowed_m = std::move(allowed); }

    [[nodiscard]] std::optional<cpu_id> cpu_for(cpu_id worker_index) const noexcept { return baitcast::detail::cpu_for(worker_index, allowed_m); }

    [[nodiscard]] bool pin_current_thread() const noexcept { return baitcast::detail::pin_current_thread(allowed_m); }

  private:
    cpu_set allowed_m;
  };
} // namespace baitcast::detail