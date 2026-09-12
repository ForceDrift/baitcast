#pragma once
#include "inplace_vector.hpp"
#include <algorithm>
#include <coroutine>
#include <cstddef>
#include <type_traits>
#include <utility>
namespace baitcast::detail {
  inline constexpr std::size_t max_children = 64;

  class task_slot {
    std::coroutine_handle<> handle_m{};

  public:
    task_slot() = default;
    explicit task_slot(std::coroutine_handle<> handle) noexcept : handle_m(handle) {}
    task_slot(task_slot &&other) noexcept : handle_m(std::exchange(other.handle_m, {})) {}
    task_slot &operator=(task_slot &&other) noexcept {
      if (this != &other) {
        if (handle_m) {
          handle_m.destroy();
        }
        handle_m = std::exchange(other.handle_m, {});
      }
      return *this;
    }
    task_slot(const task_slot &) = delete;
    task_slot &operator=(const task_slot &) = delete;
    ~task_slot() {
      if (handle_m) {
        handle_m.destroy();
      }
    }

    [[nodiscard]] bool done() const noexcept { return handle_m ? handle_m.done() : true; }

    void resume() { handle_m.resume(); }

    [[nodiscard]] std::coroutine_handle<> handle() const noexcept { return handle_m; }
  };

  static_assert(std::is_nothrow_move_constructible_v<task_slot>, "task_slot must be nothrow move constructible");
  static_assert(std::is_nothrow_destructible_v<task_slot>, "task_slot must be nothrow destructible");

  class task_registry {
    inplace_vector<task_slot, max_children> children_m{};

  public:
    task_registry() = default;

    void insert(std::coroutine_handle<> child) { children_m.emplace_back(child); }

    template <typename Predicate> std::size_t remove_if(Predicate &&pred) {
      auto new_end = std::remove_if(children_m.begin(), children_m.end(), std::forward<Predicate>(pred));
      std::size_t removed = static_cast<std::size_t>(children_m.end() - new_end);
      children_m.erase(new_end, children_m.end());
      return removed;
    }

    void clear() noexcept { children_m.clear(); }

    [[nodiscard]] bool empty() const noexcept { return children_m.empty(); }

    [[nodiscard]] std::size_t size() const noexcept { return children_m.size(); }

    [[nodiscard]] bool full() const noexcept { return children_m.full(); }

    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return max_children; }

    using iterator = typename inplace_vector<task_slot, max_children>::iterator;
    using const_iterator = typename inplace_vector<task_slot, max_children>::const_iterator;

    [[nodiscard]] iterator begin() noexcept { return children_m.begin(); }
    [[nodiscard]] iterator end() noexcept { return children_m.end(); }
    [[nodiscard]] const_iterator begin() const noexcept { return children_m.begin(); }
    [[nodiscard]] const_iterator end() const noexcept { return children_m.end(); }
    [[nodiscard]] const_iterator cbegin() const noexcept { return children_m.cbegin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return children_m.cend(); }
  };
} // namespace baitcast::detail