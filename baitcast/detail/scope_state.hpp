#pragma once
#include <cstddef>
namespace baitcast::detail {
  class scope_state {
    std::size_t active_children_m = 0;
    scope_state *parent_m = nullptr;
    bool closing_m = false;
    bool complete_m = false;

  public:
    constexpr scope_state() noexcept = default;

    constexpr void register_child() noexcept { ++active_children_m; }

    [[nodiscard]] constexpr bool child_completed() noexcept {
      if (active_children_m == 0) {
        return complete_m;
      }

      --active_children_m;
      if (closing_m && active_children_m == 0) {
        complete_m = true;
      }

      return complete_m;
    }

    constexpr void close() noexcept {
      closing_m = true;
      if (active_children_m == 0) {
        complete_m = true;
      }
    }

    constexpr void open() noexcept {
      closing_m = false;
      complete_m = false;
    }

    [[nodiscard]] constexpr bool is_closing() const noexcept { return closing_m; }

    [[nodiscard]] constexpr std::size_t active_children() const noexcept { return active_children_m; }

    [[nodiscard]] constexpr bool can_complete() const noexcept { return closing_m && active_children_m == 0; }

    [[nodiscard]] constexpr bool is_complete() const noexcept { return complete_m; }

    constexpr void set_parent(scope_state *parent) noexcept { parent_m = parent; }

    [[nodiscard]] constexpr scope_state *parent() noexcept { return parent_m; }
    [[nodiscard]] constexpr const scope_state *parent() const noexcept { return parent_m; }
  };
} // namespace baitcast::detail
