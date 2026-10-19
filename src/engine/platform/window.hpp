#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>

namespace lod
{
/// Minimum window size (320x240).
inline constexpr int32_t kWindowMinWidth = 320;
inline constexpr int32_t kWindowMinHeight = 240;

/// Window creation settings (default client size: 960x720).
struct WindowConfig
{
  std::string title = "Severed Chains"; ///< Window title.
  int32_t width = 960;                  ///< Initial client width in pixels.
  int32_t height = 720;                 ///< Initial client height in pixels.
};

/// SDL3 window (M1 foundation subset: create/show, minimum size, resize,
/// per-monitor fullscreen toggle).
///
/// Owns its `SDL_Window` (C-interop owned resource — guidelines §5); the
/// raw handle is exposed read-only via `Handle()` as the escape hatch for
/// the M2 GPU layer.
class Window
{
public:
  /// Window pixel size.
  struct Size
  {
    int32_t width = 0;  ///< Width in pixels.
    int32_t height = 0; ///< Height in pixels.
  };

  Window() = default;
  ~Window();
  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  /// Creates the window, clamped to the minimum size. Idempotent: an
  /// existing window is kept.
  ///
  /// @param config Title and initial size.
  /// @return true on success; the failure reason is logged.
  [[nodiscard]] bool Create(const WindowConfig& config);

  /// Destroys the window. Idempotent.
  void Destroy();

  /// Whether the window currently exists.
  [[nodiscard]] bool IsCreated() const;

  /// Shows the window.
  void Show();

  /// The current size in pixels.
  [[nodiscard]] Size GetSize() const;

  /// Resizes the window, clamped to the minimum size.
  void SetSize(int32_t width, int32_t height);

  /// Whether the window is in per-monitor fullscreen.
  [[nodiscard]] bool IsFullscreen() const;

  /// Toggles per-monitor fullscreen (SDL3 unified fullscreen).
  ///
  /// The Windows borderless-fullscreen +1px correction lands with the
  /// window-parity increment (requires a Windows machine to verify).
  void ToggleFullscreen();

  /// The raw SDL window (C-interop escape hatch for the M2 GPU layer).
  ///
  /// @return The handle, or nullptr when the window is not created.
  [[nodiscard]] SDL_Window* Handle() const;

private:
  SDL_Window* window_ = nullptr; // C-interop owned resource
};

} // namespace lod