# lodengine — the reusable, LoD-agnostic engine (engine-design.md).
#
# Build form: typeless (CMake's BUILD_SHARED_LIBS decides — the user's
# choice, build-system.md §1). The M1 sources are all public API
# (app/platform/util) → the whole target is PUBLIC_API (exceptions + RTTI
# disabled; build-system.md §2). When engine-internal TUs that may use RTTI
# appear, they move to an `lodengine-internal` object library (AGENTS.md RTTI
# policy).
set(ENGINE_SOURCES
  app/app.cpp
  platform/input.cpp
  platform/platform.cpp
  platform/window.cpp
  util/log.cpp
  util/status.cpp
  util/version.cpp
)

lod_add_library(lodengine PUBLIC_API SOURCES ${ENGINE_SOURCES})
target_include_directories(lodengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lodengine PUBLIC SDL3::SDL3)

# Build metadata injection (build-system.md §5).
target_compile_definitions(lodengine PRIVATE
  LOD_VERSION="${PROJECT_VERSION}"
  LOD_COMMIT_HASH="${LOD_COMMIT_HASH}"
  LOD_BUILD_TIMESTAMP="${LOD_BUILD_TIMESTAMP}"
  LOD_CHANNEL="${LOD_CHANNEL}")