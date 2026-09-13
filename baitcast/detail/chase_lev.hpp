#pragma once
#include "align.hpp"

#include <atomic>
#include <concepts>
#include <cstddef>
#include <optional>
#include <type_traits>

namespace baitcast::detail {
  template <typename T>
  concept stealable = std::is_object_v<T> && std::is_trivially_copyable_v<T> && std::is_destructible_v<T>;

  template <stealable T, std::size_t Capacity> class chase_lev {
    static_assert(Capacity >= 2, "baitcast::detail::chase_lev requires Capacity >= 2");

  public:
    using value_type = T;
    using size_type = std::size_t;

    chase_lev() = default;
    chase_lev(const chase_lev &) = delete;
    chase_lev &operator=(const chase_lev &) = delete;

    [[nodiscard]] static constexpr size_type capacity() noexcept { return Capacity; }

    // Owner-only. Returns false when the deque is full.
    [[nodiscard]] bool push_bottom(T value) noexcept {
      const size_type bottom = bottom_m.value.load(std::memory_order_relaxed);
      const size_type top = top_m.value.load(std::memory_order_acquire);
      if (bottom - top >= Capacity) {
        return false;
      }
      buffer_m[bottom % Capacity].store(value, std::memory_order_relaxed);
      std::atomic_thread_fence(std::memory_order_release);
      bottom_m.value.store(bottom + 1, std::memory_order_relaxed);
      return true;
    }

    [[nodiscard]] std::optional<T> pop_bottom() noexcept {
      const size_type bottom = bottom_m.value.load(std::memory_order_relaxed);
      if (bottom == 0) {
        return std::nullopt;
      }
      const size_type last = bottom - 1;
      bottom_m.value.store(last, std::memory_order_relaxed);
      std::atomic_thread_fence(std::memory_order_seq_cst);
      size_type top = top_m.value.load(std::memory_order_relaxed);
      if (top > last) {
        bottom_m.value.store(bottom, std::memory_order_relaxed);
        return std::nullopt;
      }
      const T value = buffer_m[last % Capacity].load(std::memory_order_relaxed);
      if (top < last) {
        return value;
      }
      if (!top_m.value.compare_exchange_strong(top, last + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
        bottom_m.value.store(bottom, std::memory_order_relaxed);
        return std::nullopt;
      }
      bottom_m.value.store(last + 1, std::memory_order_relaxed);
      return value;
    }
    [[nodiscard]] std::optional<T> steal() noexcept {
      const size_type top = top_m.value.load(std::memory_order_acquire);
      std::atomic_thread_fence(std::memory_order_seq_cst);
      const size_type bottom = bottom_m.value.load(std::memory_order_acquire);
      if (top >= bottom) {
        return std::nullopt;
      }
      const T value = buffer_m[top % Capacity].load(std::memory_order_acquire);
      size_type expected = top;
      if (!top_m.value.compare_exchange_strong(expected, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
        return std::nullopt;
      }
      return value;
    }

    [[nodiscard]] bool empty() const noexcept { return top_m.value.load(std::memory_order_acquire) >= bottom_m.value.load(std::memory_order_acquire); }

    [[nodiscard]] size_type size() const noexcept {
      const size_type top = top_m.value.load(std::memory_order_acquire);
      const size_type bottom = bottom_m.value.load(std::memory_order_acquire);
      return bottom > top ? bottom - top : 0;
    }

  private:
    cache_aligned<std::atomic<size_type>> top_m{0};
    cache_aligned<std::atomic<size_type>> bottom_m{0};
    std::atomic<T> buffer_m[Capacity];
  };
} // namespace baitcast::detail
