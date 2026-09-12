#pragma once
#include "coroutine.hpp"
#include "detail/scope_state.hpp"
#include "detail/task_registry.hpp"
#include <coroutine>
#include <cstddef>
#include <vector>
namespace baitcast {
  class nursery {
    detail::scope_state state_;
    detail::task_registry registry_;

  public:
    nursery() = default;

    nursery(const nursery &) = delete;
    nursery &operator=(const nursery &) = delete;

    ~nursery() { registry_.clear(); }

    template <typename T> void spawn(detail::coroutine<T> &&child) {
      state_.register_child();
      registry_.insert(child.release_handle());
    }

    void wait() {
      state_.close();
      while (true) {
        std::vector<std::coroutine_handle<>> pending;
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
        bool complete = false;
        for (std::size_t i = 0; i < completed; ++i) {
          complete = state_.child_completed();
        }
        if (complete) {
          break;
        }
      }
    }
  };
} // namespace baitcast
