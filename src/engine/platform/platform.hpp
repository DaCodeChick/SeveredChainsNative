#pragma once

#include <string>

namespace sc::platform
{
/// SDL3 subsystem lifecycle: video + gamepad (audio lands in M5).
///
/// Port of the reference `SdlPlatformManager` init/shutdown semantics (M1
/// foundation subset — the full input/platform parity pass is a later M1
/// increment). Owns no windows; the `Window` class does.
class SdlPlatform
{
public:
  SdlPlatform() = default;
  ~SdlPlatform();
  SdlPlatform(const SdlPlatform&) = delete;
  SdlPlatform& operator=(const SdlPlatform&) = delete;

  /// Initializes SDL3 (video + gamepad) with the project hints
  /// (parity: `SdlPlatformManager.java:149-150`).
  ///
  /// @return true on success; the failure reason is logged.
  [[nodiscard]] bool Init();

  /// Shuts SDL3 down. Idempotent.
  void Shutdown();

  /// Whether SDL3 is currently initialized.
  [[nodiscard]] bool IsInitialized() const;

  /// The last SDL error text (converted to a std::string immediately —
  /// guidelines §9.1).
  ///
  /// @return "" when there is no error.
  [[nodiscard]] std::string LastError() const;

  /// The loaded SDL3 version (e.g. "3.4.18").
  [[nodiscard]] static std::string SdlVersionString();

private:
  bool initialized_ = false;
};

} // namespace sc::platform