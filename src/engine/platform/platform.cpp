#include "platform/platform.hpp"

#include "util/log.hpp"

#include <SDL3/SDL.h>

#include <format>
#include <string>

namespace sc::platform
{
SdlPlatform::~SdlPlatform()
{
  Shutdown();
}

bool SdlPlatform::Init()
{
  if (initialized_)
  {
    return true;
  }
  // Project hints, parity: SdlPlatformManager.java:149-150.
  SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
  {
    util::Logger::Instance().LogMessage(util::LogLevel::Error, "platform", "SDL_Init failed: " + LastError());
    return false;
  }
  initialized_ = true;
  util::Logger::Instance().Log(util::LogLevel::Info, "platform", "SDL3 {} initialized (video+gamepad)",
                               SdlVersionString());
  return true;
}

void SdlPlatform::Shutdown()
{
  if (!initialized_)
  {
    return;
  }
  SDL_Quit();
  initialized_ = false;
  util::Logger::Instance().Log(util::LogLevel::Info, "platform", "SDL3 shut down");
}

bool SdlPlatform::IsInitialized() const
{
  return initialized_;
}

std::string SdlPlatform::LastError() const
{
  // C interop: convert the C string immediately (guidelines §9.1).
  return std::string(SDL_GetError());
}

std::string SdlPlatform::SdlVersionString()
{
  const int version = SDL_GetVersion();
  return std::format("{}.{}.{}", SDL_VERSIONNUM_MAJOR(version), SDL_VERSIONNUM_MINOR(version),
                     SDL_VERSIONNUM_MICRO(version));
}

} // namespace sc::platform