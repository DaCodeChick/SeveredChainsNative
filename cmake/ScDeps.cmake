# Dependency resolution (build-system.md §3): platform package manager /
# system first (find_package), pinned FetchContent release as the hermetic
# fallback. No submodules.

# --- SDL3 ---
# Preferred: find_package (Homebrew on macOS, apt/libsdl3-dev on Linux,
# vcpkg on Windows). Fallback: pinned release built from source — documented
# and used automatically when no system SDL3 is present (e.g. CI Windows).
find_package(SDL3 ${SC_SDL3_MINIMUM} QUIET)
if(NOT SDL3_FOUND)
  message(STATUS "SDL3 >= ${SC_SDL3_MINIMUM} not found on the system — fetching pinned release ${SC_SDL3_FETCH_VERSION} (build-system.md §3 fallback)")
  include(FetchContent)
  FetchContent_Declare(SDL3
    URL ${SC_SDL3_FETCH_URL}
    URL_HASH SHA256=${SC_SDL3_FETCH_SHA256}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  # Build SDL3 as a shared library (standard deployment, build-system.md §7);
  # disable its tests/examples for a lean fetch.
  set(SDL_SHARED ON CACHE BOOL "Build SDL3 shared library" FORCE)
  set(SDL_TEST OFF CACHE BOOL "Build SDL3 test suite" FORCE)
  set(SDL_EXAMPLES OFF CACHE BOOL "Build SDL3 examples" FORCE)
  FetchContent_MakeAvailable(SDL3)
endif()