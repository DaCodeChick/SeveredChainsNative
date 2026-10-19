# scengine — the reusable, LoD-agnostic engine (engine-design.md).
#
# Build form: typeless (CMake's BUILD_SHARED_LIBS decides — the user's
# choice, build-system.md §1). The M1 sources are all public API
# (app/platform/util) → the whole target is PUBLIC_API (exceptions + RTTI
# disabled; build-system.md §2). When engine-internal TUs that may use RTTI
# appear, they move to an `scengine-internal` object library (AGENTS.md RTTI
# policy).
set(ENGINE_SOURCES
  app/app.cpp
  config/config.cpp
  config/engine_configs.cpp
  config/raw_serializers.cpp
  diagnostics/crash.cpp
  platform/activation.cpp
  platform/default_bindings.cpp
  platform/input.cpp
  platform/platform.cpp
  platform/scheduler.cpp
  platform/window.cpp
  util/log.cpp
  util/status.cpp
  util/version.cpp
)

sc_add_library(scengine PUBLIC_API SOURCES ${ENGINE_SOURCES})
target_include_directories(scengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# psx (reusable PS1 tech — engine consumes it for the Tag config format;
# psx-library.md §2 consumption boundary).
target_link_libraries(scengine PUBLIC SDL3::SDL3 psx)

# Build metadata injection (build-system.md §5).
# CMake-side variables are SC_* (build system = project identity); the
# injected C++ macros are LOD_* (C++ code = game identity).
target_compile_definitions(scengine PRIVATE
  LOD_VERSION="${PROJECT_VERSION}"
  LOD_COMMIT_HASH="${SC_COMMIT_HASH}"
  LOD_BUILD_TIMESTAMP="${SC_BUILD_TIMESTAMP}"
  LOD_CHANNEL="${SC_CHANNEL}")