#include "baitcast/coroutine.hpp"
#include <cassert>
#include <iostream>

baitcast::detail::coroutine<int> produce_value() {
  co_return 42;
}

baitcast::detail::coroutine<int> consume_value() {
  int val = co_await produce_value();
  co_return val * 2;
}

int main() {
  auto task = consume_value();
  task.handle().resume();
  auto result = task.result();
  assert(result && *result == 84);
  std::cout << "result: " << *result << "\n";
  return 0;
}
