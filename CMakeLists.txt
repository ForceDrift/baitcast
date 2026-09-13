cmake_minimum_required(VERSION 3.28)
project(baitcast VERSION 0.1.0 LANGUAGES CXX)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(baitcast INTERFACE)
target_include_directories(baitcast INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(baitcast INTERFACE cxx_std_26)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
option(BAITCAST_INSTALL "Generate and install CMake package files" ${PROJECT_IS_TOP_LEVEL})

if(BAITCAST_INSTALL)
  install(TARGETS baitcast EXPORT baitcastTargets)
  install(DIRECTORY baitcast DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(
    EXPORT baitcastTargets
    FILE baitcastTargets.cmake
    NAMESPACE baitcast::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/baitcast)
  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/baitcastConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/baitcastConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/baitcast)
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/baitcastConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/baitcastConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/baitcastConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/baitcast)
endif()

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

add_executable(test_scheduler test/test_scheduler.cpp)
target_link_libraries(test_scheduler PRIVATE baitcast GTest::gtest_main)
add_test(NAME test_scheduler COMMAND test_scheduler)

add_executable(test_chase_lev test/test_chase_lev.cpp)
target_link_libraries(test_chase_lev PRIVATE baitcast GTest::gtest_main)
add_test(NAME test_chase_lev COMMAND test_chase_lev)