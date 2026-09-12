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
      static_assert(std::is_enum_v<task_status>, "task_status must be an enumeration");
      return state_m;
    }

    void set_state(task_status state) noexcept { state_m = state; }
    // add more after
  };
} // namespace baitcast::detail