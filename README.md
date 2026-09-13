# baitcast

baitcast is a high-performance **C++26 structured concurrency library**. It gives you
header-only building blocks for composing asynchronous work with coroutines:

## Requirements

- A C++26 compiler: GCC 16 (or a clang with C++26 coroutine support).
- CMake 3.28+ for building and installing.
- [optional] Compiler reflection (`-freflection` with GCC) enables reflected concurrency
  diagnostics; the library falls back to a portable implementation otherwise.

## Integration

### As a subproject / FetchContent

```cmake
add_subdirectory(path/to/baitcast)
target_link_libraries(my_app PRIVATE baitcast)
```

### Install and find_package

```sh
cmake -S . -B build
cmake --build build
cmake --install build --prefix /usr/local
```

```cmake
find_package(baitcast 0.1 REQUIRED)
target_link_libraries(my_app PRIVATE baitcast::baitcast)
```

## Features

### coroutine

`coroutine<T>` is a lazy, move-only task. It starts suspended, so the body only runs
once someone awaits it. A completed child can be inspected without awaiting:

```cpp
#include <baitcast/coroutine.hpp>

using baitcast::detail::coroutine;

coroutine<std::string> greet() {
  co_return "hello, world";
}

// Awaited inside another coroutine:
coroutine<int> user() {
  std::string message = co_await greet();
  co_return static_cast<int>(message.size());
}

coroutine<int> task = user();        // starts suspended
bool started = task.done();          // false until resumed
task.handle().resume();              // runs the whole await chain
std::optional<int> result = task.result();   // {12}
```

### nursery

`nursery` gives structured concurrency: spawn children, then wait — the destructor
blocks until every child is finished, so no work can outlive the scope.

```cpp
#include <baitcast/nursery.hpp>

coroutine<int> fetch(int id) { co_return id * 10; }

int main() {
  {
    baitcast::nursery scope;
    scope.spawn(fetch(1));
    scope.spawn(fetch(2));
    scope.spawn(fetch(3));
    scope.wait();       // every child has completed here
    return scope.is_complete() ? 0 : 1;
  }                     // dtor waits too, even on early exit
}
```

### scheduler

`submit` hands a task to a background worker; `co_await scheduler.yield()` reschedules
the current task for another round. `shutdown()` drains pending work, then joins.

```cpp
#include <baitcast/scheduler.hpp>

int main() {
  baitcast::scheduler sched;
  sched.submit(fetch(7));
  sched.submit(fetch(9));

  sched.submit([&]() -> coroutine<int> {
    for (int i = 0; i < 3; ++i) {
      co_await sched.yield();   // back to the ready queue
    }
    co_return 42;
  }());

  sched.shutdown();             // drains pending work, then joins the worker
  return sched.pending() == 0 ? 0 : 1;
}
```

### Future 

many more feautres to come soon!
