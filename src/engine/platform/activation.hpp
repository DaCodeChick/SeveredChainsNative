#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace lod
{
/// Logical keyboard key (layout-independent name) — parity set with the
/// reference `legend.core.platform.input.InputKey`.
///
/// @note The on-disk keybind format stores the **Java constant names**
///       (e.g. `NUM_0`, `KP_PLUS`, `DOUBLE_QUOTE`) via `NameOf`/`FromName`;
///       those names are format data and never change.
enum class InputKey : std::uint32_t
{
  Return,
  Escape,
  Backspace,
  Tab,
  Space,
  Exclamation,
  DoubleQuote,
  Hash,
  Dollar,
  Percent,
  Ampersand,
  Apostrophe,
  LeftParenthesis,
  RightParenthesis,
  Asterisk,
  Plus,
  Comma,
  Minus,
  Period,
  Slash,
  Num0,
  Num1,
  Num2,
  Num3,
  Num4,
  Num5,
  Num6,
  Num7,
  Num8,
  Num9,
  Colon,
  Semicolon,
  Less,
  Equals,
  Greater,
  Question,
  At,
  LeftBracket,
  Backslash,
  RightBracket,
  Caret,
  Underscore,
  Grave,
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  LeftBrace,
  Pipe,
  RightBrace,
  Tilde,
  Delete,
  PlusMinus,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  PrintScreen,
  ScrollLock,
  Pause,
  Insert,
  Home,
  PageUp,
  End,
  PageDown,
  Right,
  Left,
  Down,
  Up,
  KpDivide,
  KpMultiply,
  KpMinus,
  KpPlus,
  KpEnter,
  Kp1,
  Kp2,
  Kp3,
  Kp4,
  Kp5,
  Kp6,
  Kp7,
  Kp8,
  Kp9,
  Kp0,
  KpPeriod,
  KpEquals,
  F13,
  F14,
  F15,
  F16,
  F17,
  F18,
  F19,
  F20,
  F21,
  F22,
  F23,
  F24,
  LeftCtrl,
  LeftShift,
  LeftAlt,
  LeftGui,
  RightCtrl,
  RightShift,
  RightAlt,
  RightGui,
  /// Sentinel: number of keys.
  Count
};

/// Keyboard modifier bits (parity: `InputMod` — CTRL/ALT/SHIFT).
enum class InputMod : std::uint32_t
{
  Ctrl = 1u << 0,
  Alt = 1u << 1,
  Shift = 1u << 2
};

/// A set of keyboard modifiers (bitmask over `InputMod`).
struct InputMods
{
  std::uint32_t flags = 0; ///< Modifier bits (0 = none).

  /// Whether a modifier is present.
  [[nodiscard]] bool Has(InputMod mod) const;

  /// Whether all of `other`'s modifiers are present.
  [[nodiscard]] bool ContainsAll(const InputMods& other) const;

  /// Empty set.
  [[nodiscard]] bool Empty() const;
};

/// Logical gamepad button (layout-independent) — parity set with the
/// reference `InputButton`.
enum class InputButton : std::uint32_t
{
  DPadUp,
  DPadDown,
  DPadLeft,
  DPadRight,
  A,
  B,
  X,
  Y,
  Start,
  Select,
  LeftBumper,
  RightBumper,
  LeftStick,
  RightStick,
  Guide,
  Touchpad,
  LeftPaddle1,
  RightPaddle1,
  LeftPaddle2,
  RightPaddle2,
  Misc1,
  Misc2,
  Misc3,
  Misc4,
  Misc5,
  Misc6,
  /// Sentinel: number of buttons.
  Count
};

/// Logical gamepad axis — parity set with the reference `InputAxis`.
enum class InputAxis : std::uint32_t
{
  LeftX,
  LeftY,
  RightX,
  RightY,
  LeftTrigger,
  RightTrigger,
  /// Sentinel: number of axes.
  Count
};

/// Axis direction (parity: `InputAxisDirection` — POSITIVE=0, NEGATIVE=1).
enum class InputAxisDirection : std::uint32_t
{
  Positive = 0,
  Negative = 1
};

/// Gamepad family (drives button-name presentation, parity: `InputGamepadType`).
enum class InputGamepadType : std::uint32_t
{
  Standard,
  Xbox360,
  XboxOne,
  PlayStation,
  Switch
};

/// Which class of device produced the most recent input (parity: `InputClass`).
enum class InputClass : std::uint32_t
{
  Keyboard,
  Gamepad
};

/// How to work around Steam Input re-sending gamepad input as keyboard
/// events (parity: `IgnoreSteamInputMode`).
enum class IgnoreSteamInputMode : std::uint32_t
{
  IgnoreWhenGamepadUsed = 0,
  None
};

/// A keyboard-key activation (matches by SDL **keycode** — layout
/// dependent). Parity: `KeyInputActivation`.
struct KeyActivation
{
  InputKey key = InputKey::Escape; ///< Key.
  InputMods mods;                  ///< Required modifiers (empty = any).
};

/// A keyboard-scancode activation (matches by SDL **scancode** — layout
/// independent). Parity: `ScancodeInputActivation`.
struct ScancodeActivation
{
  InputKey key = InputKey::Escape; ///< Key (identifies the scancode).
  InputMods mods;                  ///< Required modifiers (empty = any).
};

/// A gamepad-button activation. Parity: `ButtonInputActivation`.
struct ButtonActivation
{
  InputButton button = InputButton::A; ///< Button.
};

/// A gamepad-axis activation (one direction of an axis).
/// Parity: `AxisInputActivation`.
struct AxisActivation
{
  InputAxis axis = InputAxis::LeftX;                           ///< Axis.
  InputAxisDirection direction = InputAxisDirection::Positive; ///< Direction.
};

/// Any activation (the reference `InputActivation` hierarchy as a variant).
using Activation = std::variant<KeyActivation, ScancodeActivation, ButtonActivation, AxisActivation>;

/// Keybinds value: action ID → activations, sorted by action ID (the
/// on-disk order the keybinds serializer writes).
using KeybindValue = std::vector<std::pair<std::string, std::vector<Activation>>>;

/// On-disk name of a key (the reference constant name — format data).
///
/// @param key The key (or `InputKey::Count`).
/// @return The name, or nullptr for unknown keys.
[[nodiscard]] std::string_view NameOf(InputKey key);

/// Parses a key name (case-sensitive, reference constant names).
///
/// @param name The name (e.g. "NUM_0").
/// @return The key, or nullopt.
[[nodiscard]] std::optional<InputKey> InputKeyFromName(std::string_view name);

/// On-disk name of a modifier (format data).
[[nodiscard]] std::string_view NameOf(InputMod mod);

/// Parses a modifier name.
[[nodiscard]] std::optional<InputMod> InputModFromName(std::string_view name);

/// On-disk name of a button (format data).
[[nodiscard]] std::string_view NameOf(InputButton button);

/// Parses a button name.
[[nodiscard]] std::optional<InputButton> InputButtonFromName(std::string_view name);

/// On-disk name of an axis (format data).
[[nodiscard]] std::string_view NameOf(InputAxis axis);

/// Parses an axis name.
[[nodiscard]] std::optional<InputAxis> InputAxisFromName(std::string_view name);

/// On-disk name of an axis direction (format data: "POSITIVE"/"NEGATIVE").
[[nodiscard]] std::string_view NameOf(InputAxisDirection direction);

/// Parses an axis direction name.
[[nodiscard]] std::optional<InputAxisDirection> InputAxisDirectionFromName(std::string_view name);

} // namespace lod