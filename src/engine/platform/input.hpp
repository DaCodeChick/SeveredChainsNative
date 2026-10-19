#pragma once

#include "platform/activation.hpp"
#include "platform/window.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lod
{
/// Engine-wide status codes (defined in `util/status.hpp`).
enum class Status : std::int32_t;

// ---------------------------------------------------------------------------
// SDL3 mapping (C-interop — guidelines §9.1; parity: legend.core.platform
// SdlInput — 1:1 with the SDL3 constants the reference uses)
// ---------------------------------------------------------------------------

/// The SDL3 keycode for a logical key.
///
/// @param key The logical key.
/// @return The keycode, or nullopt for keys with no SDL keycode.
[[nodiscard]] std::optional<SDL_Keycode> ToSDLKeycode(InputKey key);

/// The SDL3 scancode for a logical key.
///
/// @param key The logical key.
/// @return The scancode, or nullopt for keys with no physical scancode
///         (e.g. `InputKey::PlusMinus`).
[[nodiscard]] std::optional<SDL_Scancode> ToSDLScancode(InputKey key);

/// The logical key for an SDL3 keycode.
[[nodiscard]] std::optional<InputKey> InputKeyFromSDLKeycode(SDL_Keycode keycode);

/// The logical key for an SDL3 scancode.
[[nodiscard]] std::optional<InputKey> InputKeyFromSDLScancode(SDL_Scancode scancode);

/// The SDL3 gamepad button for a logical button.
[[nodiscard]] std::optional<SDL_GamepadButton> ToSDLGamepadButton(InputButton button);

/// The logical button for an SDL3 gamepad button.
[[nodiscard]] std::optional<InputButton> InputButtonFromSDLButton(SDL_GamepadButton button);

/// The SDL3 gamepad axis for a logical axis.
[[nodiscard]] std::optional<SDL_GamepadAxis> ToSDLGamepadAxis(InputAxis axis);

/// The logical axis for an SDL3 gamepad axis.
[[nodiscard]] std::optional<InputAxis> InputAxisFromSDLAxis(SDL_GamepadAxis axis);

/// Direction from a raw axis value (parity: `InputAxisDirection.getDirection`
/// — sign of the value).
[[nodiscard]] InputAxisDirection DirectionFromValue(std::int16_t value);

/// SDL keyboard modifier flags to logical mods (parity: `SdlInput.decodeMods`
/// — CTRL/ALT/SHIFT; other SDL mods are not part of the model).
[[nodiscard]] InputMods DecodeSDLMods(std::uint16_t sdlMods);

// ---------------------------------------------------------------------------
// Actions and bindings (parity: InputAction / InputBindings / InputBinding)
// ---------------------------------------------------------------------------

/// A logical input action's registry flags (parity: `InputAction`).
struct InputAction
{
  bool canBeEdited = false;         ///< Whether bindings may be changed in-game.
  bool visible = true;              ///< Whether it appears in the keybinds UI.
  bool useMovementDeadzone = false; ///< Deadzone set used for axis bindings.
};

/// Registry of input actions (parity: the `inputActions` registry).
///
/// Registration order is preserved (deterministic iteration for diagnostics
/// and keybinds save order).
class ActionRegistry
{
public:
  /// Registers an action.
  ///
  /// @param id The lowercase action ID (id grammar).
  /// @param action The action flags.
  /// @return `Status::Ok`, or `Status::AlreadyExists` for a duplicate ID.
  [[nodiscard]] std::expected<void, Status> Register(std::string id, InputAction action);

  /// Finds an action by ID.
  ///
  /// @return The action, or nullopt. Lifetime: this registry.
  [[nodiscard]] std::optional<std::reference_wrapper<const InputAction>> Find(std::string_view id) const;

  /// All registered actions, in registration order (ID + flags).
  [[nodiscard]] std::span<const std::pair<std::string, InputAction>> Entries() const;

  /// Number of registered actions.
  [[nodiscard]] std::size_t Size() const;

private:
  std::vector<std::pair<std::string, InputAction>> actions_;
};

/// Action → activation bindings (parity: `InputBindings` — the reference
/// keeps a per-activation-type map of binding lists; a single ordered list
/// is equivalent for matching and is deterministic).
class ActionBindings
{
public:
  /// Adds a binding (one activation → one action).
  void Add(Activation activation, std::string actionID);

  /// Replaces all bindings for an action (parity: `overwriteBindings` —
  /// clears the action's existing bindings first).
  void Overwrite(const std::string& actionID, const std::vector<Activation>& activations);

  /// The current bindings, in insertion order.
  [[nodiscard]] std::span<const std::pair<Activation, std::string>> Bindings() const;

  /// Matches a keyboard event (keycode + scancode + mods) against key and
  /// scancode bindings (parity: the reference matches key bindings by
  /// keycode and scancode bindings by scancode; required modifiers must be a
  /// subset of the pressed mods).
  ///
  /// @param keycode The SDL keycode of the event.
  /// @param scancode The SDL scancode of the event.
  /// @param mods The pressed modifiers.
  /// @param out Matched action IDs (appended; order: binding order).
  void MatchKey(SDL_Keycode keycode, SDL_Scancode scancode, const InputMods& mods, std::vector<std::string>& out) const;

  /// Matches a gamepad button event against button bindings.
  ///
  /// @param button The SDL gamepad button.
  /// @param out Matched action IDs (appended).
  void MatchButton(SDL_GamepadButton button, std::vector<std::string>& out) const;

  /// Matches a gamepad axis event against axis bindings (direction is not
  /// part of the match — it filters the value, parity: `getAxisCode`
  /// comparison).
  ///
  /// @param axis The SDL gamepad axis.
  /// @param out Matched action IDs (appended).
  void MatchAxis(SDL_GamepadAxis axis, std::vector<std::string>& out) const;

private:
  std::vector<std::pair<Activation, std::string>> bindings_;
};

/// Registers the default input actions (parity: `CoreMod`
/// `INPUT_ACTION_*` registrations — IDs and flags).
///
/// @param registry The registry (non-owning).
void RegisterDefaultInputActions(ActionRegistry& registry);

/// Registers the default action bindings (parity: `CoreMod
/// registerDefaultInputBindings` — the full default table).
///
/// @param bindings The bindings (non-owning).
void RegisterDefaultInputBindings(ActionBindings& bindings);

// ---------------------------------------------------------------------------
// Action state machine (parity: InputActionState — 500ms repeat delay,
// 100ms repeat interval)
// ---------------------------------------------------------------------------

/// Press/release/repeat state for one action (port of `InputActionState`;
/// clock-injected for testability — the reference uses `System.nanoTime()`).
class InputActionState
{
public:
  /// Repeat delay after the initial press (500ms).
  static constexpr std::int64_t RepeatDelayNs = 500'000'000LL;
  /// Repeat interval (100ms; the reference alternates REPEAT/HELD at half
  /// this period — the state machine below is the exact port).
  static constexpr std::int64_t RepeatIntervalNs = 100'000'000LL;

  /// Registers a press at `nowNs`.
  void Press(std::int64_t nowNs);

  /// Registers a release.
  void Release();

  /// Updates the axis value (axis-driven actions).
  void Axis(float axis);

  /// Advances the state machine (called once per input tick after event
  /// processing).
  ///
  /// @param nowNs The current monotonic time (ns).
  /// @return true when this call produced a repeat.
  [[nodiscard]] bool Repeat(std::int64_t nowNs);

  /// Pressed (just or fully).
  [[nodiscard]] bool IsPressed() const;

  /// Pressed or repeating.
  [[nodiscard]] bool IsRepeat() const;

  /// Not released.
  [[nodiscard]] bool IsHeld() const;

  /// The current axis value (0 when button-driven).
  [[nodiscard]] float GetAxis() const;

private:
  enum class State
  {
    Released,
    JustPressed,
    Pressed,
    Delay,
    Repeat,
    Held
  };
  State state_ = State::Released;
  float axis_ = 0.0f;
  std::int64_t timestamp_ = 0;
};

// ---------------------------------------------------------------------------
// Config access + events
// ---------------------------------------------------------------------------

/// Read-only view of the input-relevant config entries (implemented by the
/// app over `ConfigCollection`; keeps `Input` testable without a config
/// file). Parity: the reference reads `CONFIG.getConfig(...)` per event.
class InputConfigProvider
{
public:
  virtual ~InputConfigProvider() = default;

  /// The menu (on-screen control) axis deadzone, inner.
  [[nodiscard]] virtual float MenuInnerDeadzone() const = 0;
  /// The menu (on-screen control) axis deadzone, outer.
  [[nodiscard]] virtual float MenuOuterDeadzone() const = 0;
  /// The movement axis deadzone, inner.
  [[nodiscard]] virtual float MovementInnerDeadzone() const = 0;
  /// The movement axis deadzone, outer.
  [[nodiscard]] virtual float MovementOuterDeadzone() const = 0;
  /// Whether keyboard/mouse input is accepted while the window is unfocused.
  [[nodiscard]] virtual bool ReceiveInputOnInactiveWindow() const = 0;
  /// Whether all mouse input is disabled.
  [[nodiscard]] virtual bool DisableMouseInput() const = 0;
  /// The Steam Input re-send workaround mode.
  [[nodiscard]] virtual IgnoreSteamInputMode GetIgnoreSteamInputMode() const = 0;
  /// The rumble intensity scale (0..1+).
  [[nodiscard]] virtual float RumbleIntensity() const = 0;
};

/// Event callbacks for input/window events (parity: the reference
/// `WindowEvents` surface + the `InputPressed/ReleasedEvent` bus).
/// Unset callbacks are no-ops.
struct InputEvents
{
  /// Raw key press (key, scancode-or-nullopt, mods, repeat).
  std::function<void(InputKey, std::optional<InputKey>, InputMods, bool)> OnKeyPress;
  /// Raw key release.
  std::function<void(InputKey, std::optional<InputKey>, InputMods)> OnKeyRelease;
  /// Text input (the reference iterates characters).
  std::function<void(std::string_view)> OnChar;
  /// Mouse motion (relative when the capture which != 0, parity).
  std::function<void(std::int32_t, std::int32_t)> OnMouseMove;
  /// Mouse button press (SDL button index, mods).
  std::function<void(std::uint32_t, InputMods)> OnMousePress;
  /// Mouse button release.
  std::function<void(std::uint32_t, InputMods)> OnMouseRelease;
  /// Mouse wheel (x, y deltas).
  std::function<void(std::int32_t, std::int32_t)> OnMouseScroll;
  /// Gamepad button press (logical button).
  std::function<void(InputButton)> OnButtonPress;
  /// Gamepad button release.
  std::function<void(InputButton)> OnButtonRelease;
  /// Gamepad axis (logical axis, direction, menu value, movement value).
  std::function<void(InputAxis, InputAxisDirection, float, float)> OnAxis;
  /// Action pressed (action ID, repeat).
  std::function<void(std::string_view, bool)> OnActionPressed;
  /// Action released.
  std::function<void(std::string_view)> OnActionReleased;
  /// The input class changed (keyboard/gamepad).
  std::function<void(InputClass)> OnInputClassChanged;
  /// Window focus gained/lost.
  std::function<void(bool)> OnFocus;
  /// Window resized (client size in pixels).
  std::function<void(std::int32_t, std::int32_t)> OnResize;
  /// Window close requested.
  std::function<void()> OnCloseRequested;
};

// ---------------------------------------------------------------------------
// Gamepad device + Input
// ---------------------------------------------------------------------------

/// Custom deleter for an SDL gamepad handle (C interop — guidelines §5/§9.1;
/// `SDL_Gamepad` is an opaque C type, so `make_unique` cannot apply — the
/// handle is owned through this `unique_ptr` + custom deleter).
struct SDLGamepadDeleter
{
  /// Closes the gamepad (the owning resource for `SDLGamepadOwner`).
  ///
  /// @param gamepad The handle (nullptr is a no-op).
  void operator()(SDL_Gamepad* gamepad) const;
};

/// Owning wrapper for an SDL gamepad handle.
using SDLGamepadOwner = std::unique_ptr<SDL_Gamepad, SDLGamepadDeleter>;

/// An open gamepad (parity: `SdlGamepadDevice`).
struct GamepadDevice
{
  std::int32_t id = -1;                               ///< SDL joystick index.
  std::string name;                                   ///< SDL name ("Unknown" fallback, parity).
  bool rumble = false;                                ///< Rumble capability (SDL property).
  InputGamepadType type = InputGamepadType::Standard; ///< Controller family.
  SDLGamepadOwner gamepad;                            ///< Handle (unique_ptr + custom deleter).
};

/// Keyboard/mouse/gamepad input with the full action/binding model (port of
/// `SdlPlatformManager`'s input path — guidelines §9.1; the SDL event layer
/// is the C-interop boundary, everything above is action-typed).
///
/// Owns no window: it tracks focus for the single M1 window through the
/// `InputEvents::OnFocus` callbacks. **Main-thread only** (SDL requirement).
class Input
{
public:
  /// @param config The config view (non-owning; read per tick).
  /// @param events The event callbacks (copied).
  Input(InputConfigProvider& config, InputEvents events = {});

  /// Initializes input: platform hints (raw keyboard on Windows, joystick
  /// background events), the gamepad controller DB
  /// (`gamecontrollerdb.txt` in the working directory, parity), and opens
  /// any connected gamepads. Call after SDL init, before `PollEvents`.
  ///
  /// @return true on success (failure is logged).
  [[nodiscard]] bool Init();

  /// Closes all gamepads. Call before SDL shutdown.
  void Shutdown();

  /// Drains the SDL event queue and updates action state (parity:
  /// `tickInput` — run at the fixed 60Hz input rate by the scheduler):
  /// window (focus/resize/close), keyboard (raw + key/scancode bindings),
  /// text input, mouse, gamepad add/remove/axis/button, then the repeat
  /// state machine + focus-loss release + rumble lerp.
  void PollEvents();

  /// Whether an action is in the pressed set (parity: `isActionPressed`).
  [[nodiscard]] bool IsActionPressed(std::string_view actionID) const;

  /// Whether an action is pressed or repeating (parity: `isActionRepeat`).
  [[nodiscard]] bool IsActionRepeat(std::string_view actionID) const;

  /// Whether an action is held (parity: `isActionHeld`).
  [[nodiscard]] bool IsActionHeld(std::string_view actionID) const;

  /// The current axis value for an action (parity: `getAxis`).
  [[nodiscard]] float GetAxis(std::string_view actionID) const;

  /// Clears the pressed set (parity: `clearPressed` — the reference calls
  /// this after each rendered frame).
  void ClearPressed();

  /// Marks all action states for clearing (parity: `resetActionStates` —
  /// called when bindings are (re)loaded).
  void ResetActionStates();

  /// The bound actions + activations (for the keybinds config save, parity:
  /// `saveBindings` — one activation list per action with at least one
  /// binding).
  [[nodiscard]] KeybindValue SaveBindings() const;

  // --- Gamepad queries (parity: hasGamepad / getGamepadType) ---

  /// Whether any gamepad is connected.
  [[nodiscard]] bool HasGamepad() const;

  /// The most recent gamepad's family (Standard when none).
  [[nodiscard]] InputGamepadType GetGamepadType() const;

  /// The most recent gamepad's ID (-1 when none).
  [[nodiscard]] std::int32_t LastGamepadID() const;

  // --- Rumble (parity: rumble / adjustRumble / stopRumble) ---

  /// Rumbles the most recent gamepad (scaled by the rumble-intensity
  /// config, parity `SdlGamepadDevice.rumble`).
  ///
  /// @param bigIntensity 0..1 (clamped).
  /// @param smallIntensity 0..1 (clamped).
  /// @param ms Duration in milliseconds.
  void Rumble(float bigIntensity, float smallIntensity, std::int32_t ms);

  /// Lerps rumble from the current intensity to the target over `ms`
  /// (parity: `adjustRumble` — the lerp runs in `PollEvents`).
  void AdjustRumble(float bigIntensity, float smallIntensity, std::int32_t ms);

  /// Stops rumble (parity: `stopRumble`).
  void StopRumble();

  // --- Names (parity: getKeyName / getScancodeName / getButtonName /
  //     getAxisName via SDL) ---

  /// The SDL key name for a logical key.
  ///
  /// @param key The logical key.
  [[nodiscard]] std::string GetKeyName(InputKey key) const;
  /// The SDL scancode name for a logical key.
  ///
  /// @param key The logical key.
  [[nodiscard]] std::string GetScancodeName(InputKey key) const;
  /// The SDL gamepad button name for a logical button.
  ///
  /// @param button The logical button.
  [[nodiscard]] std::string GetButtonName(InputButton button) const;
  /// The SDL gamepad axis name for a logical axis.
  ///
  /// @param axis The logical axis.
  [[nodiscard]] std::string GetAxisName(InputAxis axis) const;

  /// The window focus state (for diagnostics).
  [[nodiscard]] bool HasFocus() const;

  /// The current input class (keyboard/gamepad).
  [[nodiscard]] InputClass CurrentInputClass() const;

  /// The current action bindings (read-only; for diagnostics).
  [[nodiscard]] const ActionBindings& Bindings() const;

  /// The action registry (read-only; for diagnostics).
  [[nodiscard]] const ActionRegistry& Actions() const;

private:
  // Gamepad lifecycle.
  void OpenGamepad(std::int32_t id);
  void RemoveGamepad(std::int32_t id);
  GamepadDevice* FindGamepad(std::int32_t id);
  const GamepadDevice* FindGamepad(std::int32_t id) const;

  // Action state helpers (parity: triggerBindingPress/Release,
  // getInputActionState, applyAxisActionValue, updateAxisAction).
  void TriggerBindingPress(const std::string& actionID);
  void TriggerBindingRelease(const std::string& actionID);
  InputActionState& GetActionState(const std::string& actionID);
  const InputActionState* FindActionState(std::string_view actionID) const;
  void ApplyAxisActionValue(const std::string& actionID, float value, std::int32_t gamepadID, std::int32_t axisID,
                            std::int32_t rawValue);
  void SetInputClass(InputClass cls);

  // Axis input ID packing (parity: gamepadAxisId / axisInputId — 64-bit
  // keys: gamepad id | axis id | direction).
  [[nodiscard]] static std::uint64_t GamepadAxisID(std::int32_t gamepadID, std::int32_t axisID);
  [[nodiscard]] static std::uint64_t AxisInputID(std::int32_t gamepadID, std::int32_t axisID, InputAxisDirection dir);
  [[nodiscard]] static std::int32_t AxisInputGamepadID(std::uint64_t axisInput);

  InputConfigProvider& config_;
  InputEvents events_;
  ActionRegistry actions_;
  ActionBindings bindings_;

  std::vector<GamepadDevice> gamepads_;
  std::int32_t lastGamepadID_ = -1;
  std::int32_t lastRumbleGamepadID_ = -1;

  std::vector<std::string> pressed_; ///< Action IDs in the pressed set.
  std::vector<std::pair<std::string, InputActionState>> actionStates_;
  // Axis-driven action values per (gamepad, axis, direction), max-absolute
  // (parity: AxisInputState).
  std::vector<std::pair<std::string, std::vector<std::pair<std::uint64_t, float>>>> axisActionStates_;
  std::vector<std::uint64_t> axesHeld_;
  std::vector<std::uint32_t> ignoredKeys_; ///< Scancodes swallowed by the Steam workaround.
  std::int32_t buttonsHeld_ = 0;

  bool clearActionStates_ = false;
  bool focused_ = true;
  InputClass currentInputClass_ = InputClass::Keyboard;

  // Rumble lerp state (parity: rumbleLerp* fields).
  float rumbleBigCurrent_ = 0.0f;
  float rumbleSmallCurrent_ = 0.0f;
  float rumbleBigStart_ = 0.0f;
  float rumbleSmallStart_ = 0.0f;
  float rumbleBigEnd_ = 0.0f;
  float rumbleSmallEnd_ = 0.0f;
  std::int64_t rumbleLerpStartNs_ = 0;
  std::int64_t rumbleLerpDurationNs_ = 0;
};

} // namespace lod