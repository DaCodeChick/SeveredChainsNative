#include "app/app.hpp"

#include "diagnostics/crash.hpp"
#include "util/version.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

namespace lod
{
namespace
{
/// Fixed 60Hz input rate (parity: the reference input action).
constexpr std::int32_t InputHz = 60;
/// FPS diagnostic window (ms).
constexpr std::uint64_t FpsWindowMs = 1'000;
} // namespace

App::App() : configAdapter_(configs_) {}

App::~App()
{
  window_.Destroy();
  platform_.Shutdown();
}

Status App::Run(const AppOptions& options)
{
  Logger::Instance().SetLevel(options.logLevel);
  if (!options.logFile.empty())
  {
    Logger::Instance().AttachFile(options.logFile);
  }
  // Diagnostics first (main thread, before SDL): crash handler + version
  // banner (roadmap M1: crash handler, version/commit/timestamp injection).
  CrashHandler::Install(options.logFile.string());
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

  // Config (roadmap M1: Tag-based config.dcnf reader — create/load/save,
  // presets skeleton in the config module).
  if (const auto registered = RegisterEngineConfigs(configs_); !registered.has_value())
  {
    Logger::Instance().Log(LogLevel::Error, "config", "failed to register engine config: {}",
                           ToString(registered.error()));
  }
  if (!options.configFile.empty())
  {
    if (const auto loaded = LoadGlobalConfig(configs_, options.configFile); !loaded.has_value())
    {
      Logger::Instance().Log(LogLevel::Warning, "config", "failed to load config: {}", ToString(loaded.error()));
    }
  }

  // Input (full action/binding model — the M1 input-parity increment).
  InputEvents events;
  events.OnCloseRequested = [this] { quit_ = true; };
  events.OnResize = [](std::int32_t width, std::int32_t height)
  { Logger::Instance().Log(LogLevel::Info, "window", "resized to {}x{}", width, height); };
  events.OnFocus = [](bool focused)
  { Logger::Instance().Log(LogLevel::Info, "window", focused ? "focus gained" : "focus lost"); };
  events.OnInputClassChanged = [](InputClass cls)
  {
    Logger::Instance().Log(LogLevel::Info, "input", "input class: {}",
                           cls == InputClass::Gamepad ? "gamepad" : "keyboard");
  };
  events.OnActionPressed = [this](std::string_view actionID, bool repeat)
  {
    if (actionID == "general_toggle_fullscreen" && !repeat)
    {
      window_.ToggleFullscreen();
      Logger::Instance().Log(LogLevel::Info, "window", "fullscreen toggled: {}", window_.IsFullscreen() ? "on" : "off");
    }
  };
  input_ = std::make_unique<Input>(configAdapter_, std::move(events));
  if (!input_->Init())
  {
    window_.Destroy();
    platform_.Shutdown();
    return Status::Error;
  }
  window_.Show();

  // Fixed-rate action scheduler (parity: the reference Action pair — input
  // @ 60Hz, render @ targetFps; the game reprograms rates per frame later).
  quit_ = false;
  std::uint64_t frame = 0;
  std::uint64_t fpsWindowStartNs = SDL_GetTicksNS();
  std::uint32_t fpsCount = 0;

  ScheduledAction inputTick(
    [this]
    {
      if (input_ != nullptr)
      {
        input_->PollEvents();
      }
    },
    InputHz);
  ScheduledAction renderTick(
    [&frame, &fpsCount, &fpsWindowStartNs]
    {
      // Frame hook point (no rendering yet — M2).
      ++frame;
      ++fpsCount;
      const std::uint64_t nowNs = SDL_GetTicksNS();
      if (nowNs - fpsWindowStartNs >= FpsWindowMs * 1'000'000ULL)
      {
        Logger::Instance().Log(LogLevel::Info, "app", "fps: {} (frame {})", fpsCount, frame);
        fpsWindowStartNs = nowNs;
        fpsCount = 0;
      }
    },
    std::max(options.targetFps, 1));

  while (!quit_)
  {
    inputTick.Tick();
    renderTick.Tick();
    if (options.frameCount > 0 && frame >= static_cast<std::uint64_t>(options.frameCount))
    {
      break;
    }
    // Pacing without a GPU swap (M2): yield ~1ms per loop iteration — the
    // scheduler deadlines (not the sleep) define the tick rates.
    SDL_Delay(1);
  }

  input_->ClearPressed();
  input_->Shutdown();
  input_.reset();
  window_.Destroy();
  if (!options.configFile.empty())
  {
    // Persist the config (roadmap M1: config save on shutdown; parity: the
    // reference saves on toggle + quit).
    if (const auto saved = SaveGlobalConfig(configs_, options.configFile); !saved.has_value())
    {
      Logger::Instance().Log(LogLevel::Warning, "config", "failed to save config: {}", ToString(saved.error()));
    }
  }
  platform_.Shutdown();
  Logger::Instance().Log(LogLevel::Info, "app", "shut down cleanly after {} frames", frame);
  return Status::Ok;
}

} // namespace lod
