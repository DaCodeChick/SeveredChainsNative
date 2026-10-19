# psx test suite (testing.md §2.11).
#
# Self-contained by design: uses psx's own harness (psx_test.hpp) — no
# dependency on the host repository's test framework, so the suite works in
# the main-repo build and in the standalone subproject build alike (psx is
# extractable; psx-library.md §1). psx-tests links psx only.

add_executable(psx-tests
  psx_tests_core.cpp
  psx_tests_main.cpp)
target_link_libraries(psx-tests PRIVATE psx)
target_compile_features(psx-tests PRIVATE cxx_std_23)

# Toolchain policy (build-system.md §2): exceptions disabled and warnings as
# errors on every project-owned target. Declared inline so the standalone
# build carries the same policy without the main project's ScToolchain.cmake.
if(MSVC)
  target_compile_options(psx-tests PRIVATE /EHs-c- /W4 /permissive- /WX)
else()
  target_compile_options(psx-tests PRIVATE -fno-exceptions -Wall -Wextra -Werror)
endif()

add_test(NAME psx-tests COMMAND psx-tests)
