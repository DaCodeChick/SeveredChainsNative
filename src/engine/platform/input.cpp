#include "platform/input.hpp"

#include "util/log.hpp"
#include "util/status.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace lod
{
namespace
{
/// One row of the key map: logical key ↔ SDL3 keycode + scancode. Row index
/// equals the `InputKey` value (order is pinned by the enum). `0` marks
/// "no mapping" (SDL3 unknown constants).
struct KeyRow
{
  InputKey key;
  SDL_Keycode keycode;
  SDL_Scancode scancode;
};

/// The full key map (parity: `SdlInput` — the same SDL3 constants the
/// reference maps; `PlusMinus` has no physical scancode, parity: the
/// reference imports no scancode for it).
[[nodiscard]] const std::vector<KeyRow>& KeyMap()
{
  static const std::vector<KeyRow> rows = {
    {InputKey::Return, SDLK_RETURN, SDL_SCANCODE_RETURN},
    {InputKey::Escape, SDLK_ESCAPE, SDL_SCANCODE_ESCAPE},
    {InputKey::Backspace, SDLK_BACKSPACE, SDL_SCANCODE_BACKSPACE},
    {InputKey::Tab, SDLK_TAB, SDL_SCANCODE_TAB},
    {InputKey::Space, SDLK_SPACE, SDL_SCANCODE_SPACE},
    {InputKey::Exclamation, SDLK_EXCLAIM, SDL_SCANCODE_UNKNOWN},
    {InputKey::DoubleQuote, SDLK_DBLAPOSTROPHE, SDL_SCANCODE_UNKNOWN},
    {InputKey::Hash, SDLK_HASH, SDL_SCANCODE_UNKNOWN},
    {InputKey::Dollar, SDLK_DOLLAR, SDL_SCANCODE_UNKNOWN},
    {InputKey::Percent, SDLK_PERCENT, SDL_SCANCODE_UNKNOWN},
    {InputKey::Ampersand, SDLK_AMPERSAND, SDL_SCANCODE_UNKNOWN},
    {InputKey::Apostrophe, SDLK_APOSTROPHE, SDL_SCANCODE_APOSTROPHE},
    {InputKey::LeftParenthesis, SDLK_LEFTPAREN, SDL_SCANCODE_UNKNOWN},
    {InputKey::RightParenthesis, SDLK_RIGHTPAREN, SDL_SCANCODE_UNKNOWN},
    {InputKey::Asterisk, SDLK_ASTERISK, SDL_SCANCODE_UNKNOWN},
    {InputKey::Plus, SDLK_PLUS, SDL_SCANCODE_UNKNOWN},
    {InputKey::Comma, SDLK_COMMA, SDL_SCANCODE_COMMA},
    {InputKey::Minus, SDLK_MINUS, SDL_SCANCODE_MINUS},
    {InputKey::Period, SDLK_PERIOD, SDL_SCANCODE_PERIOD},
    {InputKey::Slash, SDLK_SLASH, SDL_SCANCODE_SLASH},
    {InputKey::Num0, SDLK_0, SDL_SCANCODE_0},
    {InputKey::Num1, SDLK_1, SDL_SCANCODE_1},
    {InputKey::Num2, SDLK_2, SDL_SCANCODE_2},
    {InputKey::Num3, SDLK_3, SDL_SCANCODE_3},
    {InputKey::Num4, SDLK_4, SDL_SCANCODE_4},
    {InputKey::Num5, SDLK_5, SDL_SCANCODE_5},
    {InputKey::Num6, SDLK_6, SDL_SCANCODE_6},
    {InputKey::Num7, SDLK_7, SDL_SCANCODE_7},
    {InputKey::Num8, SDLK_8, SDL_SCANCODE_8},
    {InputKey::Num9, SDLK_9, SDL_SCANCODE_9},
    {InputKey::Colon, SDLK_COLON, SDL_SCANCODE_UNKNOWN},
    {InputKey::Semicolon, SDLK_SEMICOLON, SDL_SCANCODE_SEMICOLON},
    {InputKey::Less, SDLK_LESS, SDL_SCANCODE_UNKNOWN},
    {InputKey::Equals, SDLK_EQUALS, SDL_SCANCODE_EQUALS},
    {InputKey::Greater, SDLK_GREATER, SDL_SCANCODE_UNKNOWN},
    {InputKey::Question, SDLK_QUESTION, SDL_SCANCODE_UNKNOWN},
    {InputKey::At, SDLK_AT, SDL_SCANCODE_UNKNOWN},
    {InputKey::LeftBracket, SDLK_LEFTBRACKET, SDL_SCANCODE_LEFTBRACKET},
    {InputKey::Backslash, SDLK_BACKSLASH, SDL_SCANCODE_BACKSLASH},
    {InputKey::RightBracket, SDLK_RIGHTBRACKET, SDL_SCANCODE_RIGHTBRACKET},
    {InputKey::Caret, SDLK_CARET, SDL_SCANCODE_UNKNOWN},
    {InputKey::Underscore, SDLK_UNDERSCORE, SDL_SCANCODE_UNKNOWN},
    {InputKey::Grave, SDLK_GRAVE, SDL_SCANCODE_GRAVE},
    {InputKey::A, SDLK_A, SDL_SCANCODE_A},
    {InputKey::B, SDLK_B, SDL_SCANCODE_B},
    {InputKey::C, SDLK_C, SDL_SCANCODE_C},
    {InputKey::D, SDLK_D, SDL_SCANCODE_D},
    {InputKey::E, SDLK_E, SDL_SCANCODE_E},
    {InputKey::F, SDLK_F, SDL_SCANCODE_F},
    {InputKey::G, SDLK_G, SDL_SCANCODE_G},
    {InputKey::H, SDLK_H, SDL_SCANCODE_H},
    {InputKey::I, SDLK_I, SDL_SCANCODE_I},
    {InputKey::J, SDLK_J, SDL_SCANCODE_J},
    {InputKey::K, SDLK_K, SDL_SCANCODE_K},
    {InputKey::L, SDLK_L, SDL_SCANCODE_L},
    {InputKey::M, SDLK_M, SDL_SCANCODE_M},
    {InputKey::N, SDLK_N, SDL_SCANCODE_N},
    {InputKey::O, SDLK_O, SDL_SCANCODE_O},
    {InputKey::P, SDLK_P, SDL_SCANCODE_P},
    {InputKey::Q, SDLK_Q, SDL_SCANCODE_Q},
    {InputKey::R, SDLK_R, SDL_SCANCODE_R},
    {InputKey::S, SDLK_S, SDL_SCANCODE_S},
    {InputKey::T, SDLK_T, SDL_SCANCODE_T},
    {InputKey::U, SDLK_U, SDL_SCANCODE_U},
    {InputKey::V, SDLK_V, SDL_SCANCODE_V},
    {InputKey::W, SDLK_W, SDL_SCANCODE_W},
    {InputKey::X, SDLK_X, SDL_SCANCODE_X},
    {InputKey::Y, SDLK_Y, SDL_SCANCODE_Y},
    {InputKey::Z, SDLK_Z, SDL_SCANCODE_Z},
    {InputKey::LeftBrace, SDLK_LEFTBRACE, SDL_SCANCODE_UNKNOWN},
    {InputKey::Pipe, SDLK_PIPE, SDL_SCANCODE_UNKNOWN},
    {InputKey::RightBrace, SDLK_RIGHTBRACE, SDL_SCANCODE_UNKNOWN},
    {InputKey::Tilde, SDLK_TILDE, SDL_SCANCODE_UNKNOWN},
    {InputKey::Delete, SDLK_DELETE, SDL_SCANCODE_DELETE},
    {InputKey::PlusMinus, SDLK_PLUSMINUS, SDL_SCANCODE_UNKNOWN}, // no physical scancode (parity)
    {InputKey::F1, SDLK_F1, SDL_SCANCODE_F1},
    {InputKey::F2, SDLK_F2, SDL_SCANCODE_F2},
    {InputKey::F3, SDLK_F3, SDL_SCANCODE_F3},
    {InputKey::F4, SDLK_F4, SDL_SCANCODE_F4},
    {InputKey::F5, SDLK_F5, SDL_SCANCODE_F5},
    {InputKey::F6, SDLK_F6, SDL_SCANCODE_F6},
    {InputKey::F7, SDLK_F7, SDL_SCANCODE_F7},
    {InputKey::F8, SDLK_F8, SDL_SCANCODE_F8},
    {InputKey::F9, SDLK_F9, SDL_SCANCODE_F9},
    {InputKey::F10, SDLK_F10, SDL_SCANCODE_F10},
    {InputKey::F11, SDLK_F11, SDL_SCANCODE_F11},
    {InputKey::F12, SDLK_F12, SDL_SCANCODE_F12},
    {InputKey::PrintScreen, SDLK_PRINTSCREEN, SDL_SCANCODE_PRINTSCREEN},
    {InputKey::ScrollLock, SDLK_SCROLLLOCK, SDL_SCANCODE_SCROLLLOCK},
    {InputKey::Pause, SDLK_PAUSE, SDL_SCANCODE_PAUSE},
    {InputKey::Insert, SDLK_INSERT, SDL_SCANCODE_INSERT},
    {InputKey::Home, SDLK_HOME, SDL_SCANCODE_HOME},
    {InputKey::PageUp, SDLK_PAGEUP, SDL_SCANCODE_PAGEUP},
    {InputKey::End, SDLK_END, SDL_SCANCODE_END},
    {InputKey::PageDown, SDLK_PAGEDOWN, SDL_SCANCODE_PAGEDOWN},
    {InputKey::Right, SDLK_RIGHT, SDL_SCANCODE_RIGHT},
    {InputKey::Left, SDLK_LEFT, SDL_SCANCODE_LEFT},
    {InputKey::Down, SDLK_DOWN, SDL_SCANCODE_DOWN},
    {InputKey::Up, SDLK_UP, SDL_SCANCODE_UP},
    {InputKey::KpDivide, SDLK_KP_DIVIDE, SDL_SCANCODE_KP_DIVIDE},
    {InputKey::KpMultiply, SDLK_KP_MULTIPLY, SDL_SCANCODE_KP_MULTIPLY},
    {InputKey::KpMinus, SDLK_KP_MINUS, SDL_SCANCODE_KP_MINUS},
    {InputKey::KpPlus, SDLK_KP_PLUS, SDL_SCANCODE_KP_PLUS},
    {InputKey::KpEnter, SDLK_KP_ENTER, SDL_SCANCODE_KP_ENTER},
    {InputKey::Kp1, SDLK_KP_1, SDL_SCANCODE_KP_1},
    {InputKey::Kp2, SDLK_KP_2, SDL_SCANCODE_KP_2},
    {InputKey::Kp3, SDLK_KP_3, SDL_SCANCODE_KP_3},
    {InputKey::Kp4, SDLK_KP_4, SDL_SCANCODE_KP_4},
    {InputKey::Kp5, SDLK_KP_5, SDL_SCANCODE_KP_5},
    {InputKey::Kp6, SDLK_KP_6, SDL_SCANCODE_KP_6},
    {InputKey::Kp7, SDLK_KP_7, SDL_SCANCODE_KP_7},
    {InputKey::Kp8, SDLK_KP_8, SDL_SCANCODE_KP_8},
    {InputKey::Kp9, SDLK_KP_9, SDL_SCANCODE_KP_9},
    {InputKey::Kp0, SDLK_KP_0, SDL_SCANCODE_KP_0},
    {InputKey::KpPeriod, SDLK_KP_PERIOD, SDL_SCANCODE_KP_PERIOD},
    {InputKey::KpEquals, SDLK_KP_EQUALS, SDL_SCANCODE_KP_EQUALS},
    {InputKey::F13, SDLK_F13, SDL_SCANCODE_F13},
    {InputKey::F14, SDLK_F14, SDL_SCANCODE_F14},
    {InputKey::F15, SDLK_F15, SDL_SCANCODE_F15},
    {InputKey::F16, SDLK_F16, SDL_SCANCODE_F16},
    {InputKey::F17, SDLK_F17, SDL_SCANCODE_F17},
    {InputKey::F18, SDLK_F18, SDL_SCANCODE_F18},
    {InputKey::F19, SDLK_F19, SDL_SCANCODE_F19},
    {InputKey::F20, SDLK_F20, SDL_SCANCODE_F20},
    {InputKey::F21, SDLK_F21, SDL_SCANCODE_F21},
    {InputKey::F22, SDLK_F22, SDL_SCANCODE_F22},
    {InputKey::F23, SDLK_F23, SDL_SCANCODE_F23},
    {InputKey::F24, SDLK_F24, SDL_SCANCODE_F24},
    {InputKey::LeftCtrl, SDLK_LCTRL, SDL_SCANCODE_LCTRL},
    {InputKey::LeftShift, SDLK_LSHIFT, SDL_SCANCODE_LSHIFT},
    {InputKey::LeftAlt, SDLK_LALT, SDL_SCANCODE_LALT},
    {InputKey::LeftGui, SDLK_LGUI, SDL_SCANCODE_LGUI},
    {InputKey::RightCtrl, SDLK_RCTRL, SDL_SCANCODE_RCTRL},
    {InputKey::RightShift, SDLK_RSHIFT, SDL_SCANCODE_RSHIFT},
    {InputKey::RightAlt, SDLK_RALT, SDL_SCANCODE_RALT},
    {InputKey::RightGui, SDLK_RGUI, SDL_SCANCODE_RGUI},
  };
  return rows;
}

[[nodiscard]] const KeyRow* KeyRowFor(InputKey key)
{
  const auto index = static_cast<std::size_t>(key);
  const std::vector<KeyRow>& rows = KeyMap();
  return index < rows.size() ? &rows[index] : nullptr;
}

} // namespace

std::optional<SDL_Keycode> ToSDLKeycode(InputKey key)
{
  const KeyRow* row = KeyRowFor(key);
  if (row == nullptr || row->keycode == SDLK_UNKNOWN)
  {
    return std::nullopt;
  }
  return row->keycode;
}

std::optional<SDL_Scancode> ToSDLScancode(InputKey key)
{
  const KeyRow* row = KeyRowFor(key);
  if (row == nullptr || row->scancode == SDL_SCANCODE_UNKNOWN)
  {
    return std::nullopt;
  }
  return row->scancode;
}

std::optional<InputKey> InputKeyFromSDLKeycode(SDL_Keycode keycode)
{
  for (const auto& row : KeyMap())
  {
    if (row.keycode == keycode)
    {
      return row.key;
    }
  }
  return std::nullopt;
}

std::optional<InputKey> InputKeyFromSDLScancode(SDL_Scancode scancode)
{
  for (const auto& row : KeyMap())
  {
    if (row.scancode == scancode)
    {
      return row.key;
    }
  }
  return std::nullopt;
}

std::optional<SDL_GamepadButton> ToSDLGamepadButton(InputButton button)
{
  switch (button)
  {
    case InputButton::DPadUp:
      return SDL_GAMEPAD_BUTTON_DPAD_UP;
    case InputButton::DPadDown:
      return SDL_GAMEPAD_BUTTON_DPAD_DOWN;
    case InputButton::DPadLeft:
      return SDL_GAMEPAD_BUTTON_DPAD_LEFT;
    case InputButton::DPadRight:
      return SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
    case InputButton::A:
      return SDL_GAMEPAD_BUTTON_SOUTH;
    case InputButton::B:
      return SDL_GAMEPAD_BUTTON_EAST;
    case InputButton::X:
      return SDL_GAMEPAD_BUTTON_WEST;
    case InputButton::Y:
      return SDL_GAMEPAD_BUTTON_NORTH;
    case InputButton::Start:
      return SDL_GAMEPAD_BUTTON_START;
    case InputButton::Select:
      return SDL_GAMEPAD_BUTTON_BACK;
    case InputButton::LeftBumper:
      return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
    case InputButton::RightBumper:
      return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
    case InputButton::LeftStick:
      return SDL_GAMEPAD_BUTTON_LEFT_STICK;
    case InputButton::RightStick:
      return SDL_GAMEPAD_BUTTON_RIGHT_STICK;
    case InputButton::Guide:
      return SDL_GAMEPAD_BUTTON_GUIDE;
    case InputButton::Touchpad:
      return SDL_GAMEPAD_BUTTON_TOUCHPAD;
    case InputButton::LeftPaddle1:
      return SDL_GAMEPAD_BUTTON_LEFT_PADDLE1;
    case InputButton::RightPaddle1:
      return SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1;
    case InputButton::LeftPaddle2:
      return SDL_GAMEPAD_BUTTON_LEFT_PADDLE2;
    case InputButton::RightPaddle2:
      return SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2;
    case InputButton::Misc1:
      return SDL_GAMEPAD_BUTTON_MISC1;
    case InputButton::Misc2:
      return SDL_GAMEPAD_BUTTON_MISC2;
    case InputButton::Misc3:
      return SDL_GAMEPAD_BUTTON_MISC3;
    case InputButton::Misc4:
      return SDL_GAMEPAD_BUTTON_MISC4;
    case InputButton::Misc5:
      return SDL_GAMEPAD_BUTTON_MISC5;
    case InputButton::Misc6:
      return SDL_GAMEPAD_BUTTON_MISC6;
    case InputButton::Count:
      break;
  }
  return std::nullopt;
}

std::optional<InputButton> InputButtonFromSDLButton(SDL_GamepadButton button)
{
  switch (button)
  {
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
      return InputButton::DPadUp;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
      return InputButton::DPadDown;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
      return InputButton::DPadLeft;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
      return InputButton::DPadRight;
    case SDL_GAMEPAD_BUTTON_SOUTH:
      return InputButton::A;
    case SDL_GAMEPAD_BUTTON_EAST:
      return InputButton::B;
    case SDL_GAMEPAD_BUTTON_WEST:
      return InputButton::X;
    case SDL_GAMEPAD_BUTTON_NORTH:
      return InputButton::Y;
    case SDL_GAMEPAD_BUTTON_START:
      return InputButton::Start;
    case SDL_GAMEPAD_BUTTON_BACK:
      return InputButton::Select;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
      return InputButton::LeftBumper;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
      return InputButton::RightBumper;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK:
      return InputButton::LeftStick;
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
      return InputButton::RightStick;
    case SDL_GAMEPAD_BUTTON_GUIDE:
      return InputButton::Guide;
    case SDL_GAMEPAD_BUTTON_TOUCHPAD:
      return InputButton::Touchpad;
    case SDL_GAMEPAD_BUTTON_LEFT_PADDLE1:
      return InputButton::LeftPaddle1;
    case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1:
      return InputButton::RightPaddle1;
    case SDL_GAMEPAD_BUTTON_LEFT_PADDLE2:
      return InputButton::LeftPaddle2;
    case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2:
      return InputButton::RightPaddle2;
    case SDL_GAMEPAD_BUTTON_MISC1:
      return InputButton::Misc1;
    case SDL_GAMEPAD_BUTTON_MISC2:
      return InputButton::Misc2;
    case SDL_GAMEPAD_BUTTON_MISC3:
      return InputButton::Misc3;
    case SDL_GAMEPAD_BUTTON_MISC4:
      return InputButton::Misc4;
    case SDL_GAMEPAD_BUTTON_MISC5:
      return InputButton::Misc5;
    case SDL_GAMEPAD_BUTTON_MISC6:
      return InputButton::Misc6;
    default:
      return std::nullopt;
  }
}

std::optional<SDL_GamepadAxis> ToSDLGamepadAxis(InputAxis axis)
{
  switch (axis)
  {
    case InputAxis::LeftX:
      return SDL_GAMEPAD_AXIS_LEFTX;
    case InputAxis::LeftY:
      return SDL_GAMEPAD_AXIS_LEFTY;
    case InputAxis::RightX:
      return SDL_GAMEPAD_AXIS_RIGHTX;
    case InputAxis::RightY:
      return SDL_GAMEPAD_AXIS_RIGHTY;
    case InputAxis::LeftTrigger:
      return SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
    case InputAxis::RightTrigger:
      return SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
    case InputAxis::Count:
      break;
  }
  return std::nullopt;
}

std::optional<InputAxis> InputAxisFromSDLAxis(SDL_GamepadAxis axis)
{
  switch (axis)
  {
    case SDL_GAMEPAD_AXIS_LEFTX:
      return InputAxis::LeftX;
    case SDL_GAMEPAD_AXIS_LEFTY:
      return InputAxis::LeftY;
    case SDL_GAMEPAD_AXIS_RIGHTX:
      return InputAxis::RightX;
    case SDL_GAMEPAD_AXIS_RIGHTY:
      return InputAxis::RightY;
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
      return InputAxis::LeftTrigger;
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
      return InputAxis::RightTrigger;
    default:
      return std::nullopt;
  }
}

InputAxisDirection DirectionFromValue(std::int16_t value)
{
  return value < 0 ? InputAxisDirection::Negative : InputAxisDirection::Positive;
}

InputMods DecodeSDLMods(std::uint16_t sdlMods)
{
  InputMods mods;
  if ((sdlMods & SDL_KMOD_CTRL) != 0)
  {
    mods.flags |= static_cast<std::uint32_t>(InputMod::Ctrl);
  }
  if ((sdlMods & SDL_KMOD_ALT) != 0)
  {
    mods.flags |= static_cast<std::uint32_t>(InputMod::Alt);
  }
  if ((sdlMods & SDL_KMOD_SHIFT) != 0)
  {
    mods.flags |= static_cast<std::uint32_t>(InputMod::Shift);
  }
  return mods;
}

// ---------------------------------------------------------------------------
// ActionRegistry / ActionBindings
// ---------------------------------------------------------------------------

std::expected<void, Status> ActionRegistry::Register(std::string id, InputAction action)
{
  if (id.empty())
  {
    return std::unexpected(Status::InvalidArgument);
  }
  if (Find(id).has_value())
  {
    return std::unexpected(Status::AlreadyExists);
  }
  actions_.emplace_back(std::move(id), action);
  return {};
}

std::optional<std::reference_wrapper<const InputAction>> ActionRegistry::Find(std::string_view id) const
{
  for (const auto& [registeredID, action] : actions_)
  {
    if (registeredID == id)
    {
      return std::ref(action);
    }
  }
  return std::nullopt;
}

std::span<const std::pair<std::string, InputAction>> ActionRegistry::Entries() const
{
  return actions_;
}

std::size_t ActionRegistry::Size() const
{
  return actions_.size();
}

void ActionBindings::Add(Activation activation, std::string actionID)
{
  bindings_.emplace_back(activation, std::move(actionID));
}

void ActionBindings::Overwrite(const std::string& actionID, const std::vector<Activation>& activations)
{
  // Parity: clear the action's existing bindings first.
  std::erase_if(bindings_, [&actionID](const auto& pair) { return pair.second == actionID; });
  for (const auto& activation : activations)
  {
    bindings_.emplace_back(activation, actionID);
  }
}

std::span<const std::pair<Activation, std::string>> ActionBindings::Bindings() const
{
  return bindings_;
}

void ActionBindings::MatchKey(SDL_Keycode keycode, SDL_Scancode scancode, const InputMods& mods,
                              std::vector<std::string>& out) const
{
  for (const auto& [activation, actionID] : bindings_)
  {
    if (const KeyActivation* key = std::get_if<KeyActivation>(&activation))
    {
      if (ToSDLKeycode(key->key) == keycode && (key->mods.Empty() || mods.ContainsAll(key->mods)))
      {
        out.push_back(actionID);
      }
    }
    else if (const ScancodeActivation* scancodeActivation = std::get_if<ScancodeActivation>(&activation))
    {
      if (ToSDLScancode(scancodeActivation->key) == scancode &&
          (scancodeActivation->mods.Empty() || mods.ContainsAll(scancodeActivation->mods)))
      {
        out.push_back(actionID);
      }
    }
  }
}

void ActionBindings::MatchButton(SDL_GamepadButton button, std::vector<std::string>& out) const
{
  for (const auto& [activation, actionID] : bindings_)
  {
    if (const ButtonActivation* value = std::get_if<ButtonActivation>(&activation))
    {
      if (ToSDLGamepadButton(value->button) == button)
      {
        out.push_back(actionID);
      }
    }
  }
}

void ActionBindings::MatchAxis(SDL_GamepadAxis axis, std::vector<std::string>& out) const
{
  for (const auto& [activation, actionID] : bindings_)
  {
    if (const AxisActivation* value = std::get_if<AxisActivation>(&activation))
    {
      if (ToSDLGamepadAxis(value->axis) == axis)
      {
        out.push_back(actionID);
      }
    }
  }
}

// ---------------------------------------------------------------------------
// InputActionState (exact port of the reference state machine)
// ---------------------------------------------------------------------------

void InputActionState::Press(std::int64_t nowNs)
{
  state_ = State::JustPressed;
  axis_ = 0.0f;
  timestamp_ = nowNs;
}

void InputActionState::Release()
{
  axis_ = 0.0f;
  state_ = State::Released;
}

void InputActionState::Axis(float axis)
{
  axis_ = axis;
}

bool InputActionState::Repeat(std::int64_t nowNs)
{
  // Repeat is called after the event loop, so this stops the state from
  // immediately going to HELD.
  if (state_ == State::JustPressed)
  {
    state_ = State::Pressed;
    return false;
  }
  if (state_ == State::Pressed)
  {
    state_ = State::Delay;
    timestamp_ = nowNs;
    return false;
  }
  if (state_ == State::Delay && nowNs - timestamp_ >= RepeatDelayNs)
  {
    state_ = State::Repeat;
    timestamp_ = nowNs;
    return true;
  }
  if (state_ == State::Repeat && nowNs - timestamp_ >= RepeatIntervalNs / 2)
  {
    state_ = State::Held;
    timestamp_ = nowNs;
    return false;
  }
  if (state_ == State::Held && nowNs - timestamp_ >= RepeatIntervalNs / 2)
  {
    state_ = State::Repeat;
    timestamp_ = nowNs;
    return true;
  }
  return false;
}

bool InputActionState::IsPressed() const
{
  return state_ == State::JustPressed || state_ == State::Pressed;
}

bool InputActionState::IsRepeat() const
{
  return IsPressed() || state_ == State::Repeat;
}

bool InputActionState::IsHeld() const
{
  return state_ != State::Released;
}

float InputActionState::GetAxis() const
{
  return axis_;
}

// ---------------------------------------------------------------------------
// Input (port of SdlPlatformManager's input path)
// ---------------------------------------------------------------------------

void SDLGamepadDeleter::operator()(SDL_Gamepad* gamepad) const
{
  SDL_CloseGamepad(gamepad);
}

namespace
{
/// The monotonic clock for the input state machines (ns; the reference uses
/// System.nanoTime — parity semantics, injectable nowhere: the M1 scheduler
/// drives these at 60Hz and the 500ms/100ms repeat timing is wall-clock).
[[nodiscard]] std::int64_t NowNs()
{
  return static_cast<std::int64_t>(SDL_GetTicksNS());
}
} // namespace

Input::Input(InputConfigProvider& config, InputEvents events) : config_(config), events_(std::move(events))
{
  RegisterDefaultInputActions(actions_);
  RegisterDefaultInputBindings(bindings_);
}

bool Input::Init()
{
  // Platform hints (parity: SdlPlatformManager.init — raw keyboard on
  // Windows, joystick events in the background).
  SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

  // Controller DB (parity: gamecontrollerdb.txt in the working directory).
  if (!SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt"))
  {
    Logger::Instance().Log(LogLevel::Warning, "input", "no gamecontrollerdb.txt ({}); using built-in mappings",
                           std::string(SDL_GetError()));
  }

  gamepads_.clear();
  // Open every connected gamepad (the array is managed by SDL — no free).
  std::int32_t joystickCount = 0;
  SDL_JoystickID* joysticks = SDL_GetJoysticks(&joystickCount);
  for (std::int32_t index = 0; index < joystickCount && joysticks != nullptr; ++index)
  {
    if (SDL_GetJoystickTypeForID(joysticks[index]) == SDL_JOYSTICK_TYPE_GAMEPAD)
    {
      OpenGamepad(static_cast<std::int32_t>(joysticks[index]));
    }
  }
  return true;
}

void Input::Shutdown()
{
  gamepads_.clear(); // unique_ptr deleters close each gamepad
  lastGamepadID_ = -1;
  lastRumbleGamepadID_ = -1;
}

void Input::OpenGamepad(std::int32_t id)
{
  SDL_Gamepad* gamepad = SDL_OpenGamepad(id);
  if (gamepad == nullptr)
  {
    Logger::Instance().Log(LogLevel::Warning, "input", "failed to open gamepad {} ({})", id,
                           std::string(SDL_GetError()));
    return;
  }
  GamepadDevice device;
  device.id = id;
  device.gamepad = SDLGamepadOwner{gamepad}; // unique_ptr + custom deleter (opaque C type)
  const char* name = SDL_GetGamepadName(gamepad);
  device.name = name != nullptr ? name : "Unknown";
  if (SDL_PropertiesID props = SDL_GetGamepadProperties(gamepad); props != 0)
  {
    device.rumble = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
    SDL_DestroyProperties(props);
  }
  switch (SDL_GetGamepadType(gamepad))
  {
    case SDL_GAMEPAD_TYPE_XBOXONE:
      device.type = InputGamepadType::XboxOne;
      break;
    case SDL_GAMEPAD_TYPE_XBOX360:
      device.type = InputGamepadType::Xbox360;
      break;
    case SDL_GAMEPAD_TYPE_PS3:
    case SDL_GAMEPAD_TYPE_PS4:
    case SDL_GAMEPAD_TYPE_PS5:
      device.type = InputGamepadType::PlayStation;
      break;
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
      device.type = InputGamepadType::Switch;
      break;
    default:
      device.type = InputGamepadType::Standard;
      break;
  }
  Logger::Instance().Log(LogLevel::Info, "input", "found gamepad {}", device.name);
  gamepads_.push_back(std::move(device));
  lastGamepadID_ = id;
}

void Input::RemoveGamepad(std::int32_t id)
{
  if (GamepadDevice* device = FindGamepad(id); device != nullptr)
  {
    Logger::Instance().Log(LogLevel::Info, "input", "gamepad {} disconnected", device->name);
    if (lastGamepadID_ == id)
    {
      lastGamepadID_ = -1;
    }
    // Parity: removeGamepadAxisInputs — drop the gamepad's held axes and
    // re-evaluate each axis-driven action value.
    std::erase_if(axesHeld_, [id](std::uint64_t axisID) { return AxisInputGamepadID(axisID) == id; });
    for (auto& [actionID, values] : axisActionStates_)
    {
      const bool changed =
        std::erase_if(values, [id](const auto& pair) { return AxisInputGamepadID(pair.first) == id; }) > 0;
      if (changed)
      {
        float maxValue = 0.0f;
        for (const auto& [axisInput, value] : values)
        {
          if (std::abs(value) > std::abs(maxValue))
          {
            maxValue = value;
          }
        }
        ApplyAxisActionValue(actionID, maxValue, id, -1, 0);
      }
    }
    std::erase_if(axisActionStates_, [](const auto& pair) { return pair.second.empty(); });
    std::erase_if(gamepads_, [id](const GamepadDevice& device) { return device.id == id; });
  }
}

GamepadDevice* Input::FindGamepad(std::int32_t id)
{
  for (auto& device : gamepads_)
  {
    if (device.id == id)
    {
      return &device;
    }
  }
  return nullptr;
}

const GamepadDevice* Input::FindGamepad(std::int32_t id) const
{
  for (const auto& device : gamepads_)
  {
    if (device.id == id)
    {
      return &device;
    }
  }
  return nullptr;
}

void Input::PollEvents()
{
  while (true)
  {
    SDL_Event event{};
    if (SDL_PollEvent(&event) == 0)
    {
      break;
    }
    switch (event.type)
    {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        if (events_.OnCloseRequested)
        {
          events_.OnCloseRequested();
        }
        break;
      case SDL_EVENT_WINDOW_FOCUS_GAINED:
        focused_ = true;
        if (events_.OnFocus)
        {
          events_.OnFocus(true);
        }
        break;
      case SDL_EVENT_WINDOW_FOCUS_LOST:
        focused_ = false;
        if (events_.OnFocus)
        {
          events_.OnFocus(false);
        }
        break;
      case SDL_EVENT_WINDOW_RESIZED:
        if (events_.OnResize)
        {
          events_.OnResize(event.window.data1, event.window.data2);
        }
        break;
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
      {
        const SDL_KeyboardEvent& key = event.key;
        const bool down = event.type == SDL_EVENT_KEY_DOWN;
        if (Logger::Instance().Level() <= LogLevel::Trace)
        {
          const char* sdlName = key.key != SDLK_UNKNOWN ? SDL_GetKeyName(key.key) : "?";
          Logger::Instance().LogMessage(LogLevel::Trace, "input",
                                        down ? std::string("key down ") + sdlName : std::string("key up ") + sdlName);
        }

        // Steam Input workaround (parity: IGNORE_WHEN_GAMEPAD_USED) —
        // ignore keys while gamepad buttons/axes are held.
        if (config_.GetIgnoreSteamInputMode() == IgnoreSteamInputMode::IgnoreWhenGamepadUsed)
        {
          const bool gamepadActive = buttonsHeld_ != 0 || !axesHeld_.empty();
          if (gamepadActive)
          {
            if (down)
            {
              ignoredKeys_.push_back(static_cast<std::uint32_t>(key.scancode));
            }
            else if (std::find(ignoredKeys_.begin(), ignoredKeys_.end(), static_cast<std::uint32_t>(key.scancode)) !=
                     ignoredKeys_.end())
            {
              std::erase(ignoredKeys_, static_cast<std::uint32_t>(key.scancode));
            }
            break;
          }
        }

        SetInputClass(InputClass::Keyboard);
        const InputMods mods = DecodeSDLMods(key.mod);
        const auto inputKey = InputKeyFromSDLKeycode(key.key);
        const auto inputScan = InputKeyFromSDLScancode(key.scancode);
        if (inputKey == std::nullopt)
        {
          Logger::Instance().Log(LogLevel::Warning, "input", "unknown key {}", key.key);
        }
        if (inputKey.has_value() || inputScan.has_value())
        {
          if (down)
          {
            if (events_.OnKeyPress)
            {
              events_.OnKeyPress(*inputKey, inputScan, mods, key.repeat != 0);
            }
          }
          else
          {
            if (events_.OnKeyRelease)
            {
              events_.OnKeyRelease(*inputKey, inputScan, mods);
            }
          }
        }

        std::vector<std::string> matches;
        bindings_.MatchKey(key.key, key.scancode, mods, matches);
        for (const auto& actionID : matches)
        {
          if (down)
          {
            if (!key.repeat)
            {
              TriggerBindingPress(actionID);
            }
          }
          else
          {
            TriggerBindingRelease(actionID);
          }
        }
        break;
      }
      case SDL_EVENT_TEXT_INPUT:
      {
        // The text event does not say which device it came from — the
        // Steam workaround applies (parity).
        if (buttonsHeld_ != 0 || !axesHeld_.empty())
        {
          break;
        }
        SetInputClass(InputClass::Keyboard);
        const std::string text(reinterpret_cast<const char*>(event.text.text));
        if (events_.OnChar)
        {
          events_.OnChar(text);
        }
        break;
      }
      case SDL_EVENT_MOUSE_MOTION:
        if (!config_.DisableMouseInput())
        {
          SetInputClass(InputClass::Keyboard);
          const bool relative = event.motion.which != 0;
          if (events_.OnMouseMove)
          {
            events_.OnMouseMove(relative ? event.motion.xrel : event.motion.x,
                                relative ? event.motion.yrel : event.motion.y);
          }
        }
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP:
        if (!config_.DisableMouseInput())
        {
          SetInputClass(InputClass::Keyboard);
          const InputMods mods = DecodeSDLMods(SDL_GetModState());
          if (event.button.down)
          {
            if (events_.OnMousePress)
            {
              events_.OnMousePress(event.button.button, mods);
            }
          }
          else
          {
            if (events_.OnMouseRelease)
            {
              events_.OnMouseRelease(event.button.button, mods);
            }
          }
        }
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        if (!config_.DisableMouseInput())
        {
          SetInputClass(InputClass::Keyboard);
          if (events_.OnMouseScroll)
          {
            events_.OnMouseScroll(event.wheel.x, event.wheel.y);
          }
        }
        break;
      case SDL_EVENT_GAMEPAD_ADDED:
        OpenGamepad(event.gdevice.which);
        break;
      case SDL_EVENT_GAMEPAD_REMOVED:
        RemoveGamepad(event.gdevice.which);
        break;
      case SDL_EVENT_GAMEPAD_AXIS_MOTION:
      {
        const SDL_GamepadAxisEvent& axis = event.gaxis;
        if (!focused_ && !config_.ReceiveInputOnInactiveWindow())
        {
          break;
        }
        const float menuInner = config_.MenuInnerDeadzone();
        const float menuOuter = config_.MenuOuterDeadzone();
        const float movementInner = config_.MovementInnerDeadzone();
        const float movementOuter = config_.MovementOuterDeadzone();
        const float minInner = std::min(menuInner, movementInner);
        const std::uint64_t gamepadAxis = GamepadAxisID(axis.which, axis.axis);
        if (std::abs(axis.value) >= minInner * 0x7fff)
        {
          SetInputClass(InputClass::Gamepad);
          if (GamepadDevice* gamepad = FindGamepad(axis.which); gamepad != nullptr)
          {
            lastGamepadID_ = axis.which;
          }
          axesHeld_.push_back(gamepadAxis);
          const float menuValue =
            std::min(1.0f, (std::abs(axis.value) / 0x7fff - menuInner) / std::abs(menuOuter - menuInner));
          const float movementValue =
            std::min(1.0f, (std::abs(axis.value) / 0x7fff - movementInner) / std::abs(movementOuter - movementInner));
          if (const auto logical = InputAxisFromSDLAxis(static_cast<SDL_GamepadAxis>(axis.axis)); logical.has_value())
          {
            if (events_.OnAxis)
            {
              events_.OnAxis(*logical, DirectionFromValue(axis.value), menuValue, movementValue);
            }
          }
        }
        else
        {
          std::erase(axesHeld_, gamepadAxis);
        }

        // Parity: the reference iterates every axis binding and matches by
        // axis code; the deadzone set comes from the *action's* flag.
        for (const auto& [activation, actionID] : bindings_.Bindings())
        {
          const AxisActivation* binding = std::get_if<AxisActivation>(&activation);
          if (binding == nullptr || ToSDLGamepadAxis(binding->axis) != axis.axis)
          {
            continue;
          }
          float value = 0.0f;
          const InputAxisDirection direction = DirectionFromValue(axis.value);
          const bool useMovementDeadzone = [&]()
          {
            if (const auto action = actions_.Find(actionID); action.has_value())
            {
              return action->get().useMovementDeadzone;
            }
            return false;
          }();
          if (binding->direction == direction)
          {
            const float inner = useMovementDeadzone ? movementInner : menuInner;
            const float outer = useMovementDeadzone ? movementOuter : menuOuter;
            const float deadzoneValue =
              std::min(1.0f, (std::abs(axis.value) / 0x7fff - inner) / std::abs(outer - inner));
            if (deadzoneValue > 0.0f)
            {
              value = deadzoneValue * (axis.value < 0 ? -1.0f : 1.0f);
            }
          }
          const std::uint64_t axisInput = AxisInputID(axis.which, axis.axis, binding->direction);
          // Parity: updateAxisAction — track per-(gamepad,axis,direction)
          // values, apply the max-absolute.
          auto& values = [&]() -> std::vector<std::pair<std::uint64_t, float>>&
          {
            for (auto& [id, stored] : axisActionStates_)
            {
              if (id == actionID)
              {
                return stored;
              }
            }
            axisActionStates_.emplace_back(actionID, std::vector<std::pair<std::uint64_t, float>>{});
            return axisActionStates_.back().second;
          }();
          if (value == 0.0f)
          {
            std::erase_if(values, [axisInput](const auto& pair) { return pair.first == axisInput; });
          }
          else
          {
            bool found = false;
            for (auto& [axisKey, stored] : values)
            {
              if (axisKey == axisInput)
              {
                stored = value;
                found = true;
                break;
              }
            }
            if (!found)
            {
              values.emplace_back(axisInput, value);
            }
          }
          float maxValue = 0.0f;
          for (const auto& [axisInputKey, stored] : values)
          {
            if (std::abs(stored) > std::abs(maxValue))
            {
              maxValue = stored;
            }
          }
          ApplyAxisActionValue(actionID, maxValue, axis.which, axis.axis, axis.value);
          if (values.empty())
          {
            std::erase_if(axisActionStates_, [actionID](const auto& pair) { return pair.first == actionID; });
          }
        }
        break;
      }
      case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
      {
        ++buttonsHeld_;
        if (!focused_ && !config_.ReceiveInputOnInactiveWindow())
        {
          break;
        }
        SetInputClass(InputClass::Gamepad);
        if (GamepadDevice* gamepad = FindGamepad(event.gbutton.which); gamepad != nullptr)
        {
          lastGamepadID_ = event.gbutton.which;
        }
        if (const auto button = InputButtonFromSDLButton(static_cast<SDL_GamepadButton>(event.gbutton.button));
            button.has_value())
        {
          if (events_.OnButtonPress)
          {
            events_.OnButtonPress(*button);
          }
        }
        std::vector<std::string> matches;
        bindings_.MatchButton(static_cast<SDL_GamepadButton>(event.gbutton.button), matches);
        for (const auto& actionID : matches)
        {
          TriggerBindingPress(actionID);
        }
        break;
      }
      case SDL_EVENT_GAMEPAD_BUTTON_UP:
      {
        if (buttonsHeld_ > 0)
        {
          --buttonsHeld_;
        }
        if (!focused_ && !config_.ReceiveInputOnInactiveWindow())
        {
          break;
        }
        SetInputClass(InputClass::Gamepad);
        if (GamepadDevice* gamepad = FindGamepad(event.gbutton.which); gamepad != nullptr)
        {
          lastGamepadID_ = event.gbutton.which;
        }
        if (const auto button = InputButtonFromSDLButton(static_cast<SDL_GamepadButton>(event.gbutton.button));
            button.has_value())
        {
          if (events_.OnButtonRelease)
          {
            events_.OnButtonRelease(*button);
          }
        }
        std::vector<std::string> matches;
        bindings_.MatchButton(static_cast<SDL_GamepadButton>(event.gbutton.button), matches);
        for (const auto& actionID : matches)
        {
          TriggerBindingRelease(actionID);
        }
        break;
      }
      default:
        break;
    }
  }

  // Post-event state machine (parity: the tick tail — repeat, focus-loss
  // release, rumble lerp, deferred clear).
  const std::int64_t now = NowNs();
  if (focused_ || config_.ReceiveInputOnInactiveWindow())
  {
    for (auto& [actionID, state] : actionStates_)
    {
      if (state.Repeat(now))
      {
        if (events_.OnActionPressed)
        {
          events_.OnActionPressed(actionID, true);
        }
      }
    }
  }
  else
  {
    pressed_.clear();
    ignoredKeys_.clear();
    axesHeld_.clear();
    buttonsHeld_ = 0;
    for (auto& [actionID, state] : actionStates_)
    {
      if (state.IsHeld())
      {
        if (events_.OnActionReleased)
        {
          events_.OnActionReleased(actionID);
        }
        state.Release();
      }
    }
    axisActionStates_.clear();
  }

  if (rumbleLerpStartNs_ != 0)
  {
    const std::int64_t elapsed = now - rumbleLerpStartNs_;
    const float ratio = std::clamp(elapsed / static_cast<float>(rumbleLerpDurationNs_), 0.0f, 1.0f);
    const float big = rumbleBigStart_ + (rumbleBigEnd_ - rumbleBigStart_) * ratio;
    const float small = rumbleSmallStart_ + (rumbleSmallEnd_ - rumbleSmallStart_) * ratio;
    Rumble(big, small, 0);
    if (elapsed >= rumbleLerpDurationNs_)
    {
      rumbleLerpStartNs_ = 0;
    }
  }

  if (clearActionStates_)
  {
    clearActionStates_ = false;
    actionStates_.clear();
    axisActionStates_.clear();
  }
}

bool Input::IsActionPressed(std::string_view actionID) const
{
  return std::find(pressed_.begin(), pressed_.end(), actionID) != pressed_.end();
}

bool Input::IsActionRepeat(std::string_view actionID) const
{
  const InputActionState* state = FindActionState(actionID);
  return state != nullptr && state->IsRepeat();
}

bool Input::IsActionHeld(std::string_view actionID) const
{
  const InputActionState* state = FindActionState(actionID);
  return state != nullptr && state->IsHeld();
}

float Input::GetAxis(std::string_view actionID) const
{
  const InputActionState* state = FindActionState(actionID);
  return state != nullptr ? state->GetAxis() : 0.0f;
}

void Input::ClearPressed()
{
  pressed_.clear();
}

void Input::ResetActionStates()
{
  clearActionStates_ = true;
}

KeybindValue Input::SaveBindings() const
{
  // Parity: saveBindings — one activation list per action with bindings.
  KeybindValue keybinds;
  for (const auto& [actionID, action] : actions_.Entries())
  {
    std::vector<Activation> activations;
    for (const auto& [activation, boundAction] : bindings_.Bindings())
    {
      if (boundAction == actionID)
      {
        activations.push_back(activation);
      }
    }
    if (!activations.empty())
    {
      keybinds.emplace_back(actionID, std::move(activations));
    }
  }
  return keybinds;
}

bool Input::HasGamepad() const
{
  return !gamepads_.empty();
}

InputGamepadType Input::GetGamepadType() const
{
  const GamepadDevice* gamepad = lastGamepadID_ != -1 ? FindGamepad(lastGamepadID_) : nullptr;
  return gamepad != nullptr ? gamepad->type : InputGamepadType::Standard;
}

std::int32_t Input::LastGamepadID() const
{
  return lastGamepadID_;
}

void Input::Rumble(float bigIntensity, float smallIntensity, std::int32_t ms)
{
  rumbleBigCurrent_ = bigIntensity;
  rumbleSmallCurrent_ = smallIntensity;
  if (lastRumbleGamepadID_ != lastGamepadID_)
  {
    if (lastRumbleGamepadID_ != -1)
    {
      StopRumble();
    }
    lastRumbleGamepadID_ = lastGamepadID_;
  }
  const GamepadDevice* gamepad = lastGamepadID_ != -1 ? FindGamepad(lastGamepadID_) : nullptr;
  if (gamepad == nullptr || gamepad->gamepad == nullptr)
  {
    return;
  }
  const float scale = config_.RumbleIntensity();
  const std::uint16_t big = static_cast<std::uint16_t>(std::min(1.0f, bigIntensity * scale) * 0xffff);
  const std::uint16_t small = static_cast<std::uint16_t>(std::min(1.0f, smallIntensity * scale) * 0xffff);
  SDL_RumbleGamepad(gamepad->gamepad.get(), big, small, static_cast<std::uint32_t>(ms));
}

void Input::AdjustRumble(float bigIntensity, float smallIntensity, std::int32_t ms)
{
  rumbleBigStart_ = rumbleBigCurrent_;
  rumbleSmallStart_ = rumbleSmallCurrent_;
  rumbleBigEnd_ = bigIntensity;
  rumbleSmallEnd_ = smallIntensity;
  rumbleLerpStartNs_ = NowNs();
  rumbleLerpDurationNs_ = static_cast<std::int64_t>(ms) * 1'000'000LL;
}

void Input::StopRumble()
{
  Rumble(0.0f, 0.0f, 0);
}

std::string Input::GetKeyName(InputKey key) const
{
  if (const auto code = ToSDLKeycode(key); code.has_value())
  {
    return SDL_GetKeyName(*code);
  }
  return {};
}

std::string Input::GetScancodeName(InputKey key) const
{
  if (const auto code = ToSDLScancode(key); code.has_value())
  {
    return SDL_GetScancodeName(*code);
  }
  return {};
}

std::string Input::GetButtonName(InputButton button) const
{
  if (const auto code = ToSDLGamepadButton(button); code.has_value())
  {
    return SDL_GetGamepadStringForButton(*code);
  }
  return {};
}

std::string Input::GetAxisName(InputAxis axis) const
{
  if (const auto code = ToSDLGamepadAxis(axis); code.has_value())
  {
    return SDL_GetGamepadStringForAxis(*code);
  }
  return {};
}

bool Input::HasFocus() const
{
  return focused_;
}

InputClass Input::CurrentInputClass() const
{
  return currentInputClass_;
}

const ActionBindings& Input::Bindings() const
{
  return bindings_;
}

const ActionRegistry& Input::Actions() const
{
  return actions_;
}

void Input::TriggerBindingPress(const std::string& actionID)
{
  pressed_.push_back(actionID);
  if (events_.OnActionPressed)
  {
    events_.OnActionPressed(actionID, false);
  }
  GetActionState(actionID).Press(NowNs());
}

void Input::TriggerBindingRelease(const std::string& actionID)
{
  if (events_.OnActionReleased)
  {
    events_.OnActionReleased(actionID);
  }
  GetActionState(actionID).Release();
}

InputActionState& Input::GetActionState(const std::string& actionID)
{
  for (auto& [id, state] : actionStates_)
  {
    if (id == actionID)
    {
      return state;
    }
  }
  actionStates_.emplace_back(actionID, InputActionState{});
  return actionStates_.back().second;
}

const InputActionState* Input::FindActionState(std::string_view actionID) const
{
  for (const auto& [id, state] : actionStates_)
  {
    if (id == actionID)
    {
      return &state;
    }
  }
  return nullptr;
}

void Input::ApplyAxisActionValue(const std::string& actionID, float value, std::int32_t gamepadID, std::int32_t axisID,
                                 std::int32_t rawValue)
{
  InputActionState& state = GetActionState(actionID);
  if (value != 0.0f)
  {
    if (!state.IsHeld())
    {
      pressed_.push_back(actionID);
      if (events_.OnActionPressed)
      {
        events_.OnActionPressed(actionID, false);
      }
      state.Press(NowNs());
    }
    state.Axis(value);
  }
  else if (state.IsHeld() && state.GetAxis() != 0.0f)
  {
    (void)gamepadID;
    (void)axisID;
    (void)rawValue;
    if (events_.OnActionReleased)
    {
      events_.OnActionReleased(actionID);
    }
    state.Release();
  }
}

void Input::SetInputClass(InputClass cls)
{
  if (currentInputClass_ != cls)
  {
    currentInputClass_ = cls;
    if (events_.OnInputClassChanged)
    {
      events_.OnInputClassChanged(cls);
    }
  }
}

std::uint64_t Input::GamepadAxisID(std::int32_t gamepadID, std::int32_t axisID)
{
  return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(gamepadID)) << 32) |
         static_cast<std::uint64_t>(static_cast<std::uint32_t>(axisID));
}

std::uint64_t Input::AxisInputID(std::int32_t gamepadID, std::int32_t axisID, InputAxisDirection dir)
{
  return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(gamepadID)) << 32) |
         (static_cast<std::uint64_t>(static_cast<std::uint32_t>(axisID)) << 1) |
         static_cast<std::uint64_t>(static_cast<std::uint32_t>(dir));
}

std::int32_t Input::AxisInputGamepadID(std::uint64_t axisInput)
{
  return static_cast<std::int32_t>(axisInput >> 32);
}

} // namespace lod