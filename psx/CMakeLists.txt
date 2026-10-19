# psx — reusable PlayStation 1 technology library (psx-library.md).
#
# Identity (owner decision): PREFIX-FREE — no Sc/SC/sc in any name (target
# `psx`, namespace `psx::`); it is a separate project identity from Severed
# Chains and may be extracted to its own repository. This file is therefore
# self-contained: standalone builds work (`cmake -B build` from this
# directory) with no external toolchain files.
#
# Build form: declared WITHOUT a library type — CMake's BUILD_SHARED_LIBS
# selects it (build-system.md §1; the user's choice).
#
# Independence (psx-library.md §1): ZERO third-party dependencies — no SDL,
# no Qt, no Luau, no engine. C++23 standard library only.

cmake_minimum_required(VERSION 3.28)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  # Standalone subproject build.
  project(psx VERSION 0.1.0 LANGUAGES CXX)
  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# Test suite (self-contained; works in both integrated and standalone builds).
option(PSX_BUILD_TESTS "Build the psx test suite" ON)

set(PSX_SOURCES
  src/core/version.cpp
  src/core/error.cpp
)

# No library type: BUILD_SHARED_LIBS decides (build-system.md §1).
add_library(psx ${PSX_SOURCES})
target_include_directories(psx PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(psx PUBLIC cxx_std_23)

# Toolchain policy (build-system.md §2): psx is a public API library —
# exceptions and RTTI disabled, warnings as errors. Declared inline so the
# standalone build carries the same policy with no external toolchain file
# (the main project's toolchain policy keeps the identical flags).
if(MSVC)
  target_compile_options(psx PRIVATE /EHs-c- /GR- /W4 /permissive- /WX)
else()
  target_compile_options(psx PRIVATE -fno-exceptions -fno-rtti -Wall -Wextra -Werror)
endif()
set_target_properties(psx PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Independence audit (build-system.md §3): the zero-dep rule is enforced by
# the absence of any target_link_libraries() with an external target in this
# file. Keep it that way (CI + review gate).

include(GNUInstallDirs)
install(TARGETS psx
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(PSX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()