# Compiler policy for every project-owned target (build-system.md §2).
#
#   sc_noexcept  — exceptions disabled (MANDATORY, all targets)
#   sc_nortti    — RTTI disabled (public API targets: psx, scengine public,
#                  sc-sdk)
#   sc_warnings  — warnings as errors (project sources)
#   sc_add_library() — typeless project library (CMake's BUILD_SHARED_LIBS
#                  selects the build form — the user's choice) with the
#                  mandatory compile policy attached.

# --- sc_noexcept (every project-owned target) ---
add_library(sc_noexcept INTERFACE)
target_compile_features(sc_noexcept INTERFACE cxx_std_23)
if(MSVC)
  target_compile_options(sc_noexcept INTERFACE /EHs-c-)
else()
  target_compile_options(sc_noexcept INTERFACE -fno-exceptions)
endif()

# --- sc_nortti (public API targets) ---
add_library(sc_nortti INTERFACE)
target_link_libraries(sc_nortti INTERFACE sc_noexcept)
if(MSVC)
  target_compile_options(sc_nortti INTERFACE /GR-)
else()
  target_compile_options(sc_nortti INTERFACE -fno-rtti)
endif()

# --- sc_warnings (build with -Werror on the project sources) ---
add_library(sc_warnings INTERFACE)
if(MSVC)
  target_compile_options(sc_warnings INTERFACE /W4 /permissive- /WX)
else()
  target_compile_options(sc_warnings INTERFACE -Wall -Wextra -Werror)
endif()

# --- sc_add_library ---
# sc_add_library(<name> [PUBLIC_API] SOURCES <src>...)
# Declares a project-owned library WITHOUT a build form (CMake's
# BUILD_SHARED_LIBS decides — build-system.md §1) and applies the mandatory
# compile policy. PUBLIC_API additionally disables RTTI (public API targets).
function(sc_add_library name)
  cmake_parse_arguments(P "PUBLIC_API" "" "SOURCES" ${ARGN})
  add_library(${name} ${P_SOURCES})
  target_link_libraries(${name} PRIVATE sc_noexcept sc_warnings)
  if(P_PUBLIC_API)
    target_link_libraries(${name} PRIVATE sc_nortti)
  endif()
  set_target_properties(${name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()

# --- Configure-time compiler policy probes (build-system.md §2; M1 acceptance) ---
# 1. A TU containing `throw` must FAIL to compile under the sc_noexcept flags.
# 2. A TU using dynamic_cast must FAIL to compile under the sc_nortti flags.
# A passing probe where a failure was expected is a FATAL_ERROR (the toolchain
# is not enforcing the policy — CI regression guard). Each probe TU carries a
# preprocessor guard (_CPPRTTI/__GXX_RTTI, _CPPUNWIND/__EXCEPTIONS) that
# hard-errors when the policy is not in effect, so the probes do not depend on
# any single toolchain's diagnostic behavior.
function(sc_verify_compiler_probes)
  # Policy flags per compiler (as CMake lists — one argument per flag).
  #
  # Note (CMake 4.x): in try_compile's source form, COMPILE_DEFINITIONS
  # injects its arguments RAW into the probe's compile line (verified
  # behavior) — that is how the policy compiler flags reach the probe TU.
  #
  # MSVC nuance: /GR- degrades a dynamic_cast on a polymorphic type to
  # warning C4541 ("unpredictable behavior may result") instead of a hard
  # error (unlike Clang/GCC -fno-rtti) — /WX promotes it to an error so the
  # probe can verify the policy (the preprocessor guard in the probe TU is
  # the backstop).
  if(MSVC)
    set(throwProbeFlags "/EHs-c-")
    set(rttiProbeFlags "/EHs-c-;/GR-;/WX")
  else()
    set(throwProbeFlags "-fno-exceptions")
    set(rttiProbeFlags "-fno-exceptions;-fno-rtti")
  endif()

  # Probe 1: a TU containing `throw` under the sc_noexcept flags.
  try_compile(sc_throw_probe_ok
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/probes/throw_probe.cpp
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_DEFINITIONS ${throwProbeFlags}
    OUTPUT_VARIABLE sc_throw_probe_log)
  if(sc_throw_probe_ok)
    message(FATAL_ERROR
      "Exception policy probe FAILED: a TU using 'throw' compiled under the "
      "sc_noexcept flags. The toolchain is not enforcing exception-free "
      "compilation (expected a compile error).\nProbe build output:\n${sc_throw_probe_log}")
  endif()

  # Probe 2: a TU using dynamic_cast under the sc_nortti flags.
  try_compile(sc_rtti_probe_ok
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/probes/rtti_probe.cpp
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_DEFINITIONS ${rttiProbeFlags}
    OUTPUT_VARIABLE sc_rtti_probe_log)
  if(sc_rtti_probe_ok)
    message(FATAL_ERROR
      "RTTI policy probe FAILED: a TU using 'dynamic_cast' compiled under the "
      "sc_nortti flags. The toolchain is not enforcing RTTI-free public API "
      "compilation (expected a compile error).\nProbe build output:\n${sc_rtti_probe_log}")
  endif()

  message(STATUS "Compiler policy probes passed: 'throw' and 'dynamic_cast' are rejected by the toolchain flags")
endfunction()
sc_verify_compiler_probes()

# --- Sanitizers (dev builds only; build-system.md §5) ---
option(SC_ENABLE_ASAN "Build with AddressSanitizer (dev builds only)" OFF)
option(SC_ENABLE_UBSAN "Build with UndefinedBehaviorSanitizer (dev builds only)" OFF)
option(SC_ENABLE_TSAN "Build with ThreadSanitizer (dev builds only; audio/loader verification)" OFF)
if(SC_ENABLE_ASAN)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()
if(SC_ENABLE_UBSAN)
  add_compile_options(-fsanitize=undefined)
  add_link_options(-fsanitize=undefined)
endif()
if(SC_ENABLE_TSAN)
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()