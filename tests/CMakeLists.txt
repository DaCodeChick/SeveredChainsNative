# Engine + framework test targets (build-system.md §1; testing.md §1).

# In-repo test framework (dependency-free, no exceptions; guidelines §8.1
# registration sugar is its only macro use).
add_library(lod-test-framework STATIC
  framework/lod_test.cpp
  framework/lod_test_main.cpp)
target_include_directories(lod-test-framework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/framework)
target_link_libraries(lod-test-framework PRIVATE lod_noexcept lod_warnings)

# Engine test binary (`lod-tests` — build-system.md targets table).
add_executable(lod-tests
  engine/test_status.cpp
  engine/test_log.cpp
  engine/test_input.cpp
  engine/test_platform.cpp
  engine/test_app.cpp)
target_link_libraries(lod-tests PRIVATE lod-test-framework lodengine)
target_link_libraries(lod-tests PRIVATE lod_noexcept lod_warnings)
add_test(NAME lod-tests COMMAND lod-tests)