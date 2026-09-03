#include "baitcast/detail/align.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
namespace baitcast::detail {

  template <typename T, std::size_t Capacity = 1024> class chase_lev_deque {

  private:
    alignas(cache_line_size) std::atomic<int64_t> m_top{0};
    alignas(cache_line_size) std::atomic<int64_t> m_bottom{0};
    alignas(cache_line_size) std::vector<int64_t> buffer_{Capacity};

  public:
    chase_lev_deque() = default;
    // The owner thread reads its local write index
    //
    void push(T item) noexcept {
      int64_t b = m_bottom.load(std::memory_order_relaxed);
      int64_t t = m_top.load(std::memory_order_acquire);
      if (b - t >= static_cast<int64_t>(Capacity)) {
        // handle explansion ref: std::vector explansion
      }
      buffer_[b % Capacity] = std::move(item);
      m_bottom.store(b + 1, std::memory_order_release);
    };

    // include push to push objects using operator new
    template <typename... Args> void push(Args &&...args) noexcept {}

    std::optional<T> pop() noexcept;

    // compare_exchange_strong
    // memory_order_seq_cst
    std::optional<T> steal() noexcept;
  };

  // implement by 5th

} // namespace baitcast::detail
