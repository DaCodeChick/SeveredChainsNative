#include "app/app.hpp"

#include "util/version.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>

namespace lod
{
App::~App()
{
  window_.Destroy();
  platform_.Shutdown();
}

Status App::Run(const AppOptions& options)
{
  Logger::Instance().SetLevel(options.logLevel);
  const auto& version = GetVersionInfo();
  Logger::Instance().Log(LogLevel::Info, "app", "Severed Chains {} (commit {}, built {}, channel {})", version.version,
                         version.commit, version.buildTimestamp, version.channel);

  if (!platform_.Init())
  {
    return Status::Error;
  }
  if (!window_.Create(options.window))
  {
    platform_.Shutdown();
    return Status::Error;
  }
  window_.Show();

  const uint64_t frameIntervalMs = 1'000ULL / static_cast<uint64_t>(std::max(options.targetFps, 1));
  auto nowMs = [] { return SDL_GetTicksNS() / 1'000'000ULL; };
  uint64_t nextFrameMs = nowMs();
  uint64_t fpsWindowStartMs = nextFrameMs;
  int32_t fpsCount = 0;
  int32_t frame = 0;
  bool quit = false;

  while (!quit)
  {
    // Wait for the next event, bounded by the frame deadline (this is the
    // M1 pacing; the fixed-rate action scheduler lands with the
    // input-parity increment).
    SDL_Event event{};
    const int64_t waitMs = static_cast<int64_t>(nextFrameMs) - static_cast<int64_t>(nowMs());
    bool haveEvent =
      waitMs > 0 ? SDL_WaitEventTimeout(&event, static_cast<int32_t>(waitMs)) : SDL_PollEvent(&event) != 0;
    while (haveEvent)
    {
      HandleEvent(event, quit);
      haveEvent = SDL_PollEvent(&event) != 0;
    }
    if (quit)
    {
      break;
    }

    // Frame hook point (no rendering yet — M2).
    ++frame;
    ++fpsCount;
    const uint64_t nowMsValue = nowMs();
    if (nowMsValue - fpsWindowStartMs >= 1'000ULL)
    {
      Logger::Instance().Log(LogLevel::Info, "app", "fps: {} (frame {})", fpsCount, frame);
      fpsWindowStartMs = nowMsValue;
      fpsCount = 0;
    }

    nextFrameMs += frameIntervalMs;
    if (nextFrameMs < nowMs())
    {
      // Fell far behind (e.g. a debugger paused the process) — resync.
      nextFrameMs = nowMs();
    }
    if (options.frameCount > 0 && frame >= options.frameCount)
    {
      break;
    }
  }

  // Clean shutdown, reverse initialization order.
  window_.Destroy();
  platform_.Shutdown();
  Logger::Instance().Log(LogLevel::Info, "app", "shut down cleanly after {} frames", frame);
  return Status::Ok;
}

void App::HandleEvent(const SDL_Event& event, bool& quit)
{
  if (event.type == SDL_EVENT_WINDOW_RESIZED)
  {
    Logger::Instance().Log(LogLevel::Info, "window", "resized to {}x{}", event.window.data1, event.window.data2);
  }
  switch (input_.ProcessEvent(event))
  {
    case InputAction::Quit:
      quit = true;
      break;
    case InputAction::ToggleFullscreen:
      window_.ToggleFullscreen();
      Logger::Instance().Log(LogLevel::Info, "window", "fullscreen toggled: {}", window_.IsFullscreen() ? "on" : "off");
      break;
    default:
      break;
  }
}

} // namespace lod