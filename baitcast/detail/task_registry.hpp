#pragma once
#include <coroutine>
#include <cstddef>
#include <utility>
#include <vector>
namespace baitcast::detail {
  class task_slot {
    std::coroutine_handle<> handle_m{};

  public:
    task_slot() = default;
    explicit task_slot(std::coroutine_handle<> handle) noexcept : handle_m(handle) {}
    task_slot(task_slot&& other) noexcept : handle_m(std::exchange(other.handle_m, {})) {}
    task_slot& operator=(task_slot&& other) noexcept {
      if (this != &other) {
        if (handle_m) {
          handle_m.destroy();
        }
        handle_m = std::exchange(other.handle_m, {});
      }
      return *this;
    }
    task_slot(const task_slot&) = delete;
    task_slot& operator=(const task_slot&) = delete;
    ~task_slot() {
      if (handle_m) {
        handle_m.destroy();
      }
    }

    [[nodiscard]] bool done() const noexcept {
      return handle_m ? handle_m.done() : true;
    }

    void resume() { handle_m.resume(); }

    [[nodiscard]] std::coroutine_handle<> handle() const noexcept { return handle_m; }
  };

  class task_registry {
    std::vector<task_slot> children_m{};

  public:
    task_registry() = default;

    void insert(std::coroutine_handle<> child) { children_m.emplace_back(child); }

    template <typename Predicate> std::size_t remove_if(Predicate&& pred) {
      return std::erase_if(children_m, std::forward<Predicate>(pred));
    }

    void clear() noexcept { children_m.clear(); }

    [[nodiscard]] bool empty() const noexcept { return children_m.empty(); }

    [[nodiscard]] std::size_t size() const noexcept { return children_m.size(); }

    using iterator = typename std::vector<task_slot>::iterator;
    using const_iterator = typename std::vector<task_slot>::const_iterator;

    [[nodiscard]] iterator begin() noexcept { return children_m.begin(); }
    [[nodiscard]] iterator end() noexcept { return children_m.end(); }
    [[nodiscard]] const_iterator begin() const noexcept { return children_m.begin(); }
    [[nodiscard]] const_iterator end() const noexcept { return children_m.end(); }
    [[nodiscard]] const_iterator cbegin() const noexcept { return children_m.cbegin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return children_m.cend(); }
  };
} // namespace baitcast::detail