#include <optional>
#include <type_traits>
namespace baitcast::detail {
  enum class task_status {
    CREATED,
    RUNNING,
    SUSPENDED,
    COMPLETED,
    FAILED,
  };
  class task_state {

  private:
    task_status state_m = task_status::CREATED;

  public:
    constexpr std::optional<task_status> state() const noexcept {
      if (std::is_enum_v<task_status>) {
        return state_m;
      }

      return std::nullopt;
    }

    void set_state(task_status state) noexcept { state_m = state; }
    // add more after
  };
} // namespace baitcast::detail