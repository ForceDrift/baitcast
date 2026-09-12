#pragma once
#include "coroutine.hpp"
#include "detail/inplace_vector.hpp"
#include "detail/scope_state.hpp"
#include "detail/task_registry.hpp"
#include <coroutine>
#include <cstddef>
#include <new>
#include <stdexcept>
namespace baitcast {
  class nursery {
    detail::scope_state state_{};
    detail::task_registry registry_{};

  public:
    nursery() = default;
    nursery(const nursery &) = delete;
    nursery &operator=(const nursery &) = delete;
    nursery(nursery &&) = delete;
    nursery &operator=(nursery &&) = delete;

    ~nursery() { complete_and_wait(); }

    template <typename T> void spawn(detail::coroutine<T> &&child) {
      std::coroutine_handle<> handle = child.release_handle();
      if (!state_.can_spawn()) {
        if (handle) {
          handle.destroy();
        }
        throw std::logic_error{"baitcast::nursery: scope is closed"};
      }
      if (registry_.full()) {
        if (handle) {
          handle.destroy();
        }
        throw std::length_error{"baitcast::nursery: capacity exceeded"};
      }
      state_.register_child();
      registry_.insert(handle);
    }

    void complete() noexcept { state_.close(); }

    [[nodiscard]] bool is_complete() const noexcept { return state_.is_complete(); }

    void wait() { complete_and_wait(); }

  private:
    void complete_and_wait() {
      complete();
      while (!state_.is_complete()) {
        detail::inplace_vector<std::coroutine_handle<>, detail::max_children> pending{};
        for (detail::task_slot &slot : registry_) {
          if (!slot.done()) {
            pending.push_back(slot.handle());
          }
        }
        if (pending.empty()) {
          break;
        }
        for (std::coroutine_handle<> handle : pending) {
          handle.resume();
        }
        std::size_t completed = registry_.remove_if([](detail::task_slot &slot) { return slot.done(); });
        for (std::size_t i = 0; i < completed; ++i) {
          if (state_.child_completed()) {
            break;
          }
        }
      }
    }
  };
} // namespace baitcast