# Severed Chains Native — root CMake (build-system.md §1)
#
# CMake + Ninja; C++23 mandatory; exceptions disabled on every project-owned
# target (build-system.md §2); build form of project libraries is the user's
# choice via CMake's BUILD_SHARED_LIBS (build-system.md §1).

cmake_minimum_required(VERSION 3.28)

project(SeveredChainsNative
  VERSION 0.1.0
  DESCRIPTION "Cross-platform native C++23 reimplementation of The Legend of Dragoon: Severed Chains"
  LANGUAGES C CXX)

# C++23 is mandatory (AGENTS.md); no compiler extensions.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Predictable in-tree output layout (build-system.md §7 install layout).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(versions)
include(ScToolchain)
include(ScDeps)

if(SC_DOCS)
  include(ScDocs)
  sc_add_doxygen_target()
endif()

# Build metadata injection (build-system.md §5: no __DATE__/__TIME__ in code —
# version info comes from CMake defines).
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short=12 HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE sc_git_result
    OUTPUT_VARIABLE SC_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
if(NOT SC_COMMIT_HASH)
  set(SC_COMMIT_HASH "unknown")
endif()
string(TIMESTAMP SC_BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)

# Release-channel gating (roadmap M1): Release builds are the release
# channel; everything else is development. Carried to C++ as the
# LOD_CHANNEL macro (the SC_CHANNEL variable is CMake-side).
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(SC_CHANNEL "release")
else()
  set(SC_CHANNEL "development")
endif()
message(STATUS "Build metadata: version=${PROJECT_VERSION} commit=${SC_COMMIT_HASH} built=${SC_BUILD_TIMESTAMP} channel=${SC_CHANNEL}")

include(ScDocs)

enable_testing()

# Order note: the engine and the test framework parse before `psx` (harmless
# today — `psx/tests` is self-contained and links `psx` only; the order is
# kept for the M2+ layout where cross-subdirectory references will exist).
add_subdirectory(src/engine)
add_subdirectory(src/host)
add_subdirectory(tests)
add_subdirectory(psx)