// SDL3 platform lifecycle tests (M1: init/shutdown round-trips,
// deinit-idempotency — testing.md §2.1). Headless via the dummy video
// driver.
#include "lod_test.hpp"

#include <SDL3/SDL.h>

#include <platform/platform.hpp>
#include <string>

LOD_TEST(PlatformInitShutdownRoundTrip, "platform")
{
  // Headless video driver (set before SDL_Init; consumed there).
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
  lod::SDLPlatform platform;
  LOD_REQUIRE(!platform.IsInitialized());
  LOD_REQUIRE(platform.Init());
  LOD_REQUIRE(platform.IsInitialized());
  // Idempotent init.
  LOD_REQUIRE(platform.Init());
  // Version string is well-formed (converted from the C API — guidelines §9.1).
  const std::string version = platform.VersionString();
  LOD_REQUIRE(version.starts_with("3."));
  platform.Shutdown();
  LOD_REQUIRE(!platform.IsInitialized());
  // Idempotent shutdown.
  platform.Shutdown();
  LOD_REQUIRE(!platform.IsInitialized());
  return true;
}

LOD_TEST(PlatformReinitAfterShutdown, "platform")
{
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
  lod::SDLPlatform platform;
  LOD_REQUIRE(platform.Init());
  platform.Shutdown();
  // SDL3 supports re-init after a full quit (test-process lifecycle).
  LOD_REQUIRE(platform.Init());
  platform.Shutdown();
  return true;
}