# Compiler policy for every project-owned target (build-system.md §2).
#
#   lod_noexcept  — exceptions disabled (MANDATORY, all targets)
#   lod_nortti    — RTTI disabled (public API targets: psx, lodengine public,
#                  lod-sdk)
#   lod_warnings  — warnings as errors (project sources)
#   lod_add_library() — typeless project library (CMake's BUILD_SHARED_LIBS
#                  selects the build form — the user's choice) with the
#                  mandatory compile policy attached.

# --- lod_noexcept (every project-owned target) ---
add_library(lod_noexcept INTERFACE)
target_compile_features(lod_noexcept INTERFACE cxx_std_23)
if(MSVC)
  target_compile_options(lod_noexcept INTERFACE /EHs-c-)
else()
  target_compile_options(lod_noexcept INTERFACE -fno-exceptions)
endif()

# --- lod_nortti (public API targets) ---
add_library(lod_nortti INTERFACE)
target_link_libraries(lod_nortti INTERFACE lod_noexcept)
if(MSVC)
  target_compile_options(lod_nortti INTERFACE /GR-)
else()
  target_compile_options(lod_nortti INTERFACE -fno-rtti)
endif()

# --- lod_warnings (build with -Werror on the project sources) ---
add_library(lod_warnings INTERFACE)
if(MSVC)
  target_compile_options(lod_warnings INTERFACE /W4 /permissive- /WX)
else()
  target_compile_options(lod_warnings INTERFACE -Wall -Wextra -Werror)
endif()

# --- lod_add_library ---
# lod_add_library(<name> [PUBLIC_API] SOURCES <src>...)
# Declares a project-owned library WITHOUT a build form (CMake's
# BUILD_SHARED_LIBS decides — build-system.md §1) and applies the mandatory
# compile policy. PUBLIC_API additionally disables RTTI (public API targets).
function(lod_add_library name)
  cmake_parse_arguments(P "PUBLIC_API" "" "SOURCES" ${ARGN})
  add_library(${name} ${P_SOURCES})
  target_link_libraries(${name} PRIVATE lod_noexcept lod_warnings)
  if(P_PUBLIC_API)
    target_link_libraries(${name} PRIVATE lod_nortti)
  endif()
  set_target_properties(${name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()

# --- Configure-time compiler policy probes (build-system.md §2; M1 acceptance) ---
# 1. A TU containing `throw` must FAIL to compile under the lod_noexcept flags.
# 2. A TU using dynamic_cast must FAIL to compile under the lod_nortti flags.
# A passing probe where a failure was expected is a FATAL_ERROR (the toolchain
# is not enforcing the policy — CI regression guard).
function(lod_verify_compiler_probes)
  # Policy flags per compiler (as CMake lists — one argument per flag).
  #
  # Note (CMake 4.x): in try_compile's source form, COMPILE_DEFINITIONS
  # injects its arguments RAW into the probe's compile line (verified
  # behavior) — that is how the policy compiler flags reach the probe TU.
  if(MSVC)
    set(throwProbeFlags "/EHs-c-")
    set(rttiProbeFlags "/EHs-c-;/GR-")
  else()
    set(throwProbeFlags "-fno-exceptions")
    set(rttiProbeFlags "-fno-exceptions;-fno-rtti")
  endif()

  # Probe 1: a TU containing `throw` under the lod_noexcept flags.
  try_compile(lod_throw_probe_ok
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/probes/throw_probe.cpp
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_DEFINITIONS ${throwProbeFlags})
  if(lod_throw_probe_ok)
    message(FATAL_ERROR
      "Exception policy probe FAILED: a TU using 'throw' compiled under the "
      "lod_noexcept flags. The toolchain is not enforcing exception-free "
      "compilation (expected a compile error).")
  endif()

  # Probe 2: a TU using dynamic_cast under the lod_nortti flags.
  try_compile(lod_rtti_probe_ok
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/probes/rtti_probe.cpp
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_DEFINITIONS ${rttiProbeFlags})
  if(lod_rtti_probe_ok)
    message(FATAL_ERROR
      "RTTI policy probe FAILED: a TU using 'dynamic_cast' compiled under the "
      "lod_nortti flags. The toolchain is not enforcing RTTI-free public API "
      "compilation (expected a compile error).")
  endif()

  message(STATUS "Compiler policy probes passed: 'throw' and 'dynamic_cast' are rejected by the toolchain flags")
endfunction()
lod_verify_compiler_probes()

# --- Sanitizers (dev builds only; build-system.md §5) ---
option(LOD_ENABLE_ASAN "Build with AddressSanitizer (dev builds only)" OFF)
option(LOD_ENABLE_UBSAN "Build with UndefinedBehaviorSanitizer (dev builds only)" OFF)
option(LOD_ENABLE_TSAN "Build with ThreadSanitizer (dev builds only; audio/loader verification)" OFF)
if(LOD_ENABLE_ASAN)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()
if(LOD_ENABLE_UBSAN)
  add_compile_options(-fsanitize=undefined)
  add_link_options(-fsanitize=undefined)
endif()
if(LOD_ENABLE_TSAN)
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()