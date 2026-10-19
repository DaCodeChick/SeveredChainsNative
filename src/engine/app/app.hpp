#pragma once

#include "platform/input.hpp"
#include "platform/platform.hpp"
#include "platform/window.hpp"
#include "util/log.hpp"
#include "util/status.hpp"

namespace lod
{
/// Application options (M1 foundation).
struct AppOptions
{
  /// Auto-quit after N frames (-1: run until a quit action).
  int32_t frameCount = -1;
  /// Target frame rate for the event-loop pacing (frames per second).
  int32_t targetFps = 60;
  /// Window settings (title/size; the minimum size is enforced).
  WindowConfig window;
  /// Minimum log level.
  LogLevel logLevel = LogLevel::Info;
};

/// The application: owns the platform, window, and input, runs the event
/// loop with frame pacing, and shuts down in reverse initialization order
/// (roadmap M1). No rendering yet — the frame hook point arrives with M2.
class App
{
public:
  App() = default;
  ~App();
  App(const App&) = delete;
  App& operator=(const App&) = delete;

  /// Runs until a quit action (or `frameCount` is reached).
  ///
  /// @param options Application options (defaults: run until quit, 60 fps).
  /// @return `Status::Ok` on a clean shutdown, or the first fatal error
  ///         (the failure is logged).
  [[nodiscard]] Status Run(const AppOptions& options = {});

private:
  /// Handles one event: resize diagnostics + action dispatch.
  void HandleEvent(const SDL_Event& event, bool& quit);

  SDLPlatform platform_;
  Window window_;
  Input input_;
};

} // namespace lod