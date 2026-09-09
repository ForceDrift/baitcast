#include <coroutine>
#include <cstddef>
#include <exception>
#include <future>
#include <memory>
#include <optional>
#include <utility>
namespace baitcast::detail {

  class [[nodiscard]] coroutine {
  public:
    struct promise_type {

      std::exception_ptr exception_m{nullptr};
      coroutine get_ret_obj() noexcept { return coroutine{handle_type::from_promise(*this)}; }
      std::suspend_always initial_suspend() const noexcept { return std::suspend_always{}; } // impl when task<T> is impl
      std::suspend_always final_suspend() const noexcept { return std::suspend_always{}; }   // impl when task<T>  is impl
      std::optional<std::exception_ptr> unhandled_exception() const noexcept { return std::current_exception(); };
    };

    using handle_type = std::coroutine_handle<promise_type>;

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
    bool done() const noexcept { return handle_m ? handle_m.done() : true; }

    [[nodiscard]] std::optional<std::exception_ptr> exception() const noexcept {
      if (handle_m && handle_m.promise().exception_m) {
        return handle_m.promise().exception_m;
      }
      return std::nullopt;
    };
  };

} // namespace baitcast::detail
