#include "baitcast/detail/align.hpp"

int main() {
  static_assert(baitcast::detail::cache_line_size > 0,
                "Cache line size must be positive");
  baitcast::detail::cache_aligned<int> val{};
  return 0;
}
