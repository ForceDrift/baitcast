#include "detail/task_state.hpp"
#include <coroutine>
#include <cstddef>
#include <exception>
#include <optional>
#include <utility>
namespace baitcast::detail {
  template <typename T> class [[nodiscard]] coroutine {
  public:
    struct promise_type;

    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {

      task_state task_state_m{};
      std::optional<T> value_m{std::nullopt};
      std::exception_ptr exception_m{nullptr};
      std::coroutine_handle<> caller_m{nullptr};

      [[nodiscard]] coroutine get_return_object() noexcept { return coroutine{handle_type::from_promise(*this)}; }
      [[nodiscard]] std::suspend_always initial_suspend() noexcept {
        task_state_m.set_state(task_status::SUSPENDED);
        return std::suspend_always{};
      } // impl when task<T> is impl

      struct final_awaiter {
        [[nodiscard]] bool await_ready() const noexcept { return false; }
        [[nodiscard]] std::coroutine_handle<> await_suspend(handle_type h) const noexcept { return h.promise().caller_m ? h.promise().caller_m : std::noop_coroutine(); }
        void await_resume() const noexcept {}
      };

      [[nodiscard]] final_awaiter final_suspend() const noexcept { return {}; }
      void return_value(T value) {
        value_m = std::move(value);
        task_state_m.set_state(task_status::COMPLETED);
      }
      void unhandled_exception() noexcept {
        exception_m = std::current_exception();
        task_state_m.set_state(task_status::FAILED);
      }
    };

  private:
    handle_type handle_m{nullptr};

  public:
    coroutine(handle_type handle) noexcept : handle_m(handle) {};
    explicit coroutine(coroutine &&other) noexcept : handle_m(std::exchange(other.handle_m, nullptr)) {}

    coroutine &operator=(coroutine &&other) noexcept {
      if (this != &other) {
        if (handle_m) {
          handle_m.destroy();
        }
        handle_m = std::exchange(other.handle_m, nullptr);
      }
      return *this;
    }

    ~coroutine() {
      if (handle_m) {
        handle_m.destroy();
      }
    };

    coroutine(const coroutine &) = delete;
    coroutine &operator=(const coroutine &) = delete;
    [[nodiscard]] bool done() const noexcept { return handle_m ? handle_m.done() : true; }

    [[nodiscard]] bool await_ready() const noexcept { return handle_m && handle_m.done(); }
    [[nodiscard]] std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
      handle_m.promise().task_state_m.set_state(task_status::RUNNING);
      handle_m.promise().caller_m = caller;
      return handle_m;
    }
    [[nodiscard]] T await_resume() {
      if (handle_m.promise().exception_m) {
        std::rethrow_exception(handle_m.promise().exception_m);
      }
      return std::move(*handle_m.promise().value_m);
    }

    [[nodiscard]] handle_type handle() noexcept { return handle_m; }

    handle_type release_handle() noexcept { return std::exchange(handle_m, nullptr); }

    [[nodiscard]] std::optional<task_status> state() const noexcept { return handle_m ? handle_m.promise().task_state_m.state() : std::nullopt; }

    [[nodiscard]] std::optional<T> result() const {
      if (handle_m.promise().exception_m) {
        std::rethrow_exception(handle_m.promise().exception_m);
      }
      return handle_m.promise().value_m;
    }

    [[nodiscard]] std::optional<std::exception_ptr> exception() const noexcept {
      if (handle_m && handle_m.promise().exception_m) {
        return handle_m.promise().exception_m;
      }
      return std::nullopt;
    };
  };

} // namespace baitcast::detail
