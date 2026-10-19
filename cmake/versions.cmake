# Pinned dependency versions and project options — single source of truth
# (build-system.md §1/§3).

# --- Project options (build-system.md §1) ---
option(LOD_TESTS "Build the test suites (ctest targets)" ON)
option(LOD_DOCS "Create the Doxygen API documentation target (lod-api-docs)" ON)
option(LOD_SANITIZERS
     "Build with ASan+UBSan (debug builds only — performance-sensitive code is never instrumented; build-system.md §1)"
     OFF)

# --- SDL3 ---
# Resolution strategy (build-system.md §3): platform package manager / system
# first (find_package: Homebrew on macOS, apt on Linux, vcpkg on Windows);
# pinned FetchContent release as the hermetic fallback (CI Windows, or any
# machine without a system SDL3).
# Minimum acceptable SDL3 for the architecture (build-system.md §6).
set(LOD_SDL3_MINIMUM "3.2.0")
# FetchContent fallback pin (official release asset; verified SHA256).
set(LOD_SDL3_FETCH_VERSION "3.4.18")
set(LOD_SDL3_FETCH_URL
    "https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.tar.gz")
set(LOD_SDL3_FETCH_SHA256 "9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3")