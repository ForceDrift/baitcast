cmake_minimum_required(VERSION 3.28)
project(baitcast LANGUAGES CXX)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(baitcast INTERFACE)
target_include_directories(baitcast INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(baitcast INTERFACE cxx_std_26)

include(FetchContent)
set(FETCHCONTENT_QUIET ON)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(test_align test/test_align.cpp)
target_link_libraries(test_align PRIVATE baitcast GTest::gtest_main)
add_test(NAME test_align COMMAND test_align)

add_executable(test_coroutine test/test_coroutine.cpp)
target_link_libraries(test_coroutine PRIVATE baitcast GTest::gtest_main)
add_test(NAME test_coroutine COMMAND test_coroutine)

add_executable(test_scope_state test/test_scope_state.cpp)
target_link_libraries(test_scope_state PRIVATE baitcast GTest::gtest_main)
add_test(NAME test_scope_state COMMAND test_scope_state)

add_executable(test_task_registry test/test_task_registry.cpp)
target_link_libraries(test_task_registry PRIVATE baitcast GTest::gtest_main)
add_test(NAME test_task_registry COMMAND test_task_registry)

add_executable(test_nursery test/test_nursery.cpp)
target_link_libraries(test_nursery PRIVATE baitcast GTest::gtest_main)
add_test(NAME test_nursery COMMAND test_nursery)

add_executable(test_inplace_vector test/test_inplace_vector.cpp)
target_link_libraries(test_inplace_vector PRIVATE baitcast GTest::gtest_main)
add_test(NAME test_inplace_vector COMMAND test_inplace_vector)

add_executable(test_scheduler_state test/test_scheduler_state.cpp)
target_link_libraries(test_scheduler_state PRIVATE baitcast GTest::gtest_main)
add_test(NAME test_scheduler_state COMMAND test_scheduler_state)

add_executable(test_worker test/test_worker.cpp)
target_link_libraries(test_worker PRIVATE baitcast GTest::gtest_main)
add_test(NAME test_worker COMMAND test_worker)