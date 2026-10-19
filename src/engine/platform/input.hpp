#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <string_view>

namespace lod
{
/// Game actions (M1 foundation subset; the full key/button binding model
/// lands in a later M1 increment).
enum class InputAction : std::int32_t
{
  None = 0,
  /// Quit the application (ESC / window close).
  Quit,
  /// Toggle per-monitor fullscreen (F11).
  ToggleFullscreen,
  /// Sentinel: number of actions.
  Count
};

/// Name of an action (diagnostics).
///
/// @param action The action.
/// @return A `std::string_view` backed by a process-lifetime static string.
[[nodiscard]] std::string_view ToString(InputAction action);

/// Keyboard input state + action bindings (M1 foundation subset).
///
/// Event-driven: actions update as `SDL_Event`s are processed (no polling).
/// `SDL_Event` is the C-interop boundary type (guidelines §9.1); above this
/// class the API is action-typed only.
class Input
{
public:
  /// Processes one input event and updates the held-action state.
  ///
  /// @param event The SDL input event (keyboard/window events).
  /// @return The action the event triggered, or `InputAction::None`.
  [[nodiscard]] InputAction ProcessEvent(const SDL_Event& event);

  /// Whether an action is currently held (key-down, not yet released).
  [[nodiscard]] bool IsActionHeld(InputAction action) const;

private:
  /// Maps a physical key to its bound action (the M1 binding table).
  [[nodiscard]] static InputAction ActionForKey(SDL_Keycode key);

  std::array<bool, static_cast<std::size_t>(InputAction::Count)> held_{};
};

} // namespace lod