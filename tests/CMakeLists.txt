# Engine + framework test targets (build-system.md §1; testing.md §1).

# In-repo test framework (dependency-free, no exceptions; guidelines §8.1
# registration sugar is its only macro use).
add_library(sc-test-framework STATIC
  framework/lod_test.cpp
  framework/lod_test_main.cpp)
target_include_directories(sc-test-framework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/framework)
target_link_libraries(sc-test-framework PRIVATE sc_noexcept sc_warnings)

# Engine test binary (`sc-tests` — build-system.md targets table).
add_executable(sc-tests
  engine/test_status.cpp
  engine/test_log.cpp
  engine/test_input.cpp
  engine/test_platform.cpp
  engine/test_app.cpp)
target_link_libraries(sc-tests PRIVATE sc-test-framework scengine)
target_link_libraries(sc-tests PRIVATE sc_noexcept sc_warnings)
add_test(NAME sc-tests COMMAND sc-tests)