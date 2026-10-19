#include "platform/platform.hpp"

#include "util/log.hpp"

#include <SDL3/SDL.h>

#include <format>
#include <string>

namespace lod
{
SDLPlatform::~SDLPlatform()
{
  Shutdown();
}

bool SDLPlatform::Init()
{
  if (initialized_)
  {
    return true;
  }
  // Project hints: raw keyboard enabled on Windows; allow joystick events
  // while the window is unfocused.
  SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
  {
    Logger::Instance().LogMessage(LogLevel::Error, "platform", "SDL_Init failed: " + LastError());
    return false;
  }
  initialized_ = true;
  Logger::Instance().Log(LogLevel::Info, "platform", "SDL3 {} initialized (video+gamepad)", VersionString());
  return true;
}

void SDLPlatform::Shutdown()
{
  if (!initialized_)
  {
    return;
  }
  SDL_Quit();
  initialized_ = false;
  Logger::Instance().Log(LogLevel::Info, "platform", "SDL3 shut down");
}

bool SDLPlatform::IsInitialized() const
{
  return initialized_;
}

std::string SDLPlatform::LastError() const
{
  // C interop: convert the C string immediately (guidelines §9.1).
  return std::string(SDL_GetError());
}

std::string SDLPlatform::VersionString()
{
  const int version = SDL_GetVersion();
  return std::format("{}.{}.{}", SDL_VERSIONNUM_MAJOR(version), SDL_VERSIONNUM_MINOR(version),
                     SDL_VERSIONNUM_MICRO(version));
}

} // namespace lod