#pragma once

#include "config/config.hpp"
#include "platform/input.hpp"
#include "platform/platform.hpp"
#include "platform/scheduler.hpp"
#include "platform/window.hpp"
#include "util/log.hpp"
#include "util/status.hpp"

#include <filesystem>

namespace lod
{
/// Application options (M1 foundation).
struct AppOptions
{
  /// Auto-quit after N render frames (-1: run until a close request).
  std::int32_t frameCount = -1;
  /// Render action rate in Hz (the input action is fixed at 60Hz — parity).
  std::int32_t targetFps = 60;
  /// Window settings (title/size; the minimum size is enforced).
  WindowConfig window;
  /// Minimum log level.
  LogLevel logLevel = LogLevel::Info;
  /// Tag-based global config file ("config.dcnf"; "" = no config file).
  std::filesystem::path configFile = "config.dcnf";
  /// Application log file ("debug.log"; "" = console only). The crash
  /// handler appends its report here.
  std::filesystem::path logFile = "debug.log";
};

/// The application: owns the platform, window, config, and input; runs the
/// fixed-rate action scheduler (input @ 60Hz + render @ targetFps — parity
/// `SdlPlatformManager.init` + `PlatformManager.run`) and shuts down in
/// reverse initialization order (roadmap M1). No rendering yet — the frame
/// hook point arrives with M2.
class App
{
public:
  App();
  ~App();
  App(const App&) = delete;
  App& operator=(const App&) = delete;

  /// Runs until a window close request (or `frameCount` render frames).
  ///
  /// @param options Application options.
  /// @return `Status::Ok` on a clean shutdown, or the first fatal error
  ///         (the failure is logged).
  [[nodiscard]] Status Run(const AppOptions& options = {});

private:
  SDLPlatform platform_;
  Window window_;
  ConfigCollection configs_;
  /// Config view for the input subsystem (see Run for the mapping).
  struct ConfigInputAdapter : InputConfigProvider
  {
    explicit ConfigInputAdapter(const ConfigCollection& configs) : configs_(configs) {}
    [[nodiscard]] float MenuInnerDeadzone() const override { return configs_.GetFloat("menu_inner_deadzone", 0.5f); }
    [[nodiscard]] float MenuOuterDeadzone() const override { return configs_.GetFloat("menu_outer_deadzone", 0.6f); }
    [[nodiscard]] float MovementInnerDeadzone() const override
    {
      return configs_.GetFloat("movement_inner_deadzone", 0.2f);
    }
    [[nodiscard]] float MovementOuterDeadzone() const override
    {
      return configs_.GetFloat("movement_outer_deadzone", 0.9f);
    }
    [[nodiscard]] bool ReceiveInputOnInactiveWindow() const override
    {
      return configs_.GetBool("receive_input_on_inactive_window", false);
    }
    [[nodiscard]] bool DisableMouseInput() const override { return configs_.GetBool("disable_mouse_input", false); }
    [[nodiscard]] IgnoreSteamInputMode GetIgnoreSteamInputMode() const override
    {
      return configs_.GetString("ignore_steam_input_mode", "IGNORE_WHEN_GAMEPAD_USED") == "NONE"
               ? IgnoreSteamInputMode::None
               : IgnoreSteamInputMode::IgnoreWhenGamepadUsed;
    }
    [[nodiscard]] float RumbleIntensity() const override { return configs_.GetFloat("rumble_intensity", 1.0f); }
    const ConfigCollection& configs_;
  };
  ConfigInputAdapter configAdapter_;
  std::unique_ptr<Input> input_;
  bool quit_ = false;
};

} // namespace lod