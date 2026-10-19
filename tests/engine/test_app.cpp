// App smoke test (roadmap M1 deliverable: host executable initializes
// SDL3, runs an event loop, and shuts down cleanly — headless).
#include "sc_test.hpp"

#include <SDL3/SDL.h>

#include <app/app.hpp>
#include <util/status.hpp>
#include <util/version.hpp>

SC_TEST(AppHeadlessSmokeRun, "app")
{
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
  sc::app::App app;
  sc::app::AppOptions options;
  options.frameCount = 5;
  options.logLevel = sc::util::LogLevel::Info;
  SC_REQUIRE(app.Run(options) == sc::Status::Ok);
  return true;
}

SC_TEST(AppVersionInjection, "app")
{
  // Build metadata injection (build-system.md §5): fields are populated in
  // the tree build (defaults only guard IDE builds).
  const auto& version = sc::util::GetVersionInfo();
  SC_REQUIRE(!version.version.empty());
  SC_REQUIRE(!version.commit.empty());
  SC_REQUIRE(!version.buildTimestamp.empty());
  SC_REQUIRE(version.channel == "development" || version.channel == "release");
  return true;
}