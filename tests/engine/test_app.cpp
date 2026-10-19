// App smoke test (roadmap M1 deliverable: host executable initializes
// SDL3, runs an event loop, and shuts down cleanly — headless).
#include "lod_test.hpp"

#include <SDL3/SDL.h>

#include <app/app.hpp>
#include <util/status.hpp>
#include <util/version.hpp>

LOD_TEST(AppHeadlessSmokeRun, "app")
{
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
  lod::App app;
  lod::AppOptions options;
  options.frameCount = 5;
  options.logLevel = lod::LogLevel::Info;
  LOD_REQUIRE(app.Run(options) == lod::Status::Ok);
  return true;
}

LOD_TEST(AppVersionInjection, "app")
{
  // Build metadata injection (build-system.md §5): fields are populated in
  // the tree build (defaults only guard IDE builds).
  const auto& version = lod::GetVersionInfo();
  LOD_REQUIRE(!version.version.empty());
  LOD_REQUIRE(!version.commit.empty());
  LOD_REQUIRE(!version.buildTimestamp.empty());
  LOD_REQUIRE(version.channel == "development" || version.channel == "release");
  return true;
}