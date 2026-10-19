// App smoke test (roadmap M1 deliverable: host executable initializes
// SDL3, runs the fixed-rate input/render scheduler, and shuts down
// cleanly — headless).
#include "lod_test.hpp"

#include <SDL3/SDL.h>

#include <app/app.hpp>
#include <filesystem>
#include <util/status.hpp>
#include <util/version.hpp>

LOD_TEST(AppHeadlessSmokeRun, "app")
{
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
  lod::App app;
  lod::AppOptions options;
  options.frameCount = 5;
  options.logLevel = lod::LogLevel::Info;
  // Keep the config/log files out of the build directory (and the run
  // deterministic): scratch dir under the system temp.
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / "lod-sc-tests" / "app";
  std::filesystem::create_directories(dir);
  options.configFile = dir / "config.dcnf";
  options.logFile = dir / "debug.log";
  std::filesystem::remove(options.configFile);
  std::filesystem::remove(options.logFile);
  LOD_REQUIRE(app.Run(options) == lod::Status::Ok);
  // The config file was created and is loadable (the save-on-shutdown path).
  lod::ConfigCollection configs;
  LOD_REQUIRE(lod::RegisterEngineConfigs(configs).has_value());
  LOD_REQUIRE(lod::LoadGlobalConfig(configs, options.configFile).has_value());
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